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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>
#include <errno.h>

#include "errLogLib.h"
#include "mfileObj.h"
#include "shrMLib.h"
#include "shrexpinfo.h"
#include "expentrystructs.h"
#include "hostAcqStructs.h"
#include "chanLib.h"
#include "commfuncs.h"
#include "eventHandler.h"

typedef struct _ptrNsize_ {
		     char *pCode;
	             unsigned int cSize;
	           } CODEENTRY;

typedef CODEENTRY *CODELIST;

		    
extern int chanId;	/* Channel Id */
SHR_EXP_INFO expInfo = NULL;   /* start address of shared Exp. Info Structure */

// static char command[256];
static char response[256];
static char dspdlstat[256];
static SHR_MEM_ID  ShrExpInfo = NULL;
ExpEntryInfo ActiveExpInfo;

/* global info retained for acode shipping to console */
static char labelBase[81];
static int nAcodes;
static MFILE_ID mapAcodes = NULL;
static CODELIST CodeList = NULL;
static int AbortXfer = 0;	/* SIGUSR2 will set this to abort transfer */

void resetState();
int mapInExp(ExpEntryInfo *expid);
int mapOutExp(ExpEntryInfo *expid);
int sendExpStat(char *str);
int sendRTFile(char *filename,char *bufRootName);
int sendTblFiles(unsigned int nTables, char *filename,char *bufRootName);
int sendTable(char *fname, int tab_nbr, char *tname);
int sendWFFile(char *filename,char *bufRootName);
int sendAcodes(int nCodes, char *filename,char *bufRootName);
int sendCommand(char *command,int trsize,char *response,int recvsiz);
int blockAbortTransfer();
int unblockAbortTransfer();
int writeToConsole(int chanId,char* bufAdr,int size);
int getResponce(char *response,int recvsiz);
void install_ref(int size,int bufn,unsigned char *ptr);

extern int deliverMessage( char *interface, char *message );
/***************************************************************
* sendCodes - Send an Experiment down to console
* 	      RT parameter table, Tables, Acodes
*   Args.   1. ExpInfo File
*	    2. Basename for Named Buffers (Exp1, Exp1rt,Exp1f1,Exp1t1,etc.)
*
*/
int sendCodes(char *argstr)
{
    char *bufRootName;
    char *filename;
    int result;

    /* Close Out any previous Experiment Files */
    resetState();
    AbortXfer = 0;

    /* 1st Arg Exp Info File */
    filename = strtok(NULL," ");
    strcpy(ActiveExpInfo.ExpId,filename);

    /* 2nd Arg. Basename (e.g. Exp1)  */
    bufRootName = strtok(NULL," ");

    DPRINT2(1,"sendCodes: Info File: '%s', Base name: '%s'\n",
	        ActiveExpInfo.ExpId,bufRootName);

    /* Map In the Experiments Info File */
    if (mapInExp(&ActiveExpInfo) == -1)
    {
        errLogRet(ErrLogOp,debugInfo,
		"sendCodes: Exp: '%s' failed no Codes sent.\n",
		 ActiveExpInfo.ExpId);
        /* Need some type of msge back to Expproc to indicate failure */
	return(-1);
    }

    if (ActiveExpInfo.ExpInfo->InteractiveFlag == 1)
       ActiveExpInfo.ExpInfo->ExpState = 0;
    if (ActiveExpInfo.ExpInfo->ExpState >= EXPSTATE_TERMINATE)
    {
       DPRINT1(1,"Terminate before we got started, ExpState: %d\n",
		ActiveExpInfo.ExpInfo->ExpState);
       mapOutExp(&ActiveExpInfo);
       return(0);
    }

    /* E.G. 
    Format:Cmd BufType Label(base) Size(max) number start_number 
	   RT parms - "downLoad,Dynamic,Exp1,64,1,0"
	   Tables   - "downLoad,Dynamic,Exp1,1024,1,0"
	   Tables   - "downLoad,Dynamic,Exp1,1024,1,1"
	   Acodes   - "downLoad,Fixed,Exp1,512,10,0"
	   Acodes   - "downLoad,Fixed,Exp1,512,10,10"
    */
    result = sendExpStat("startExp");
    if (result < 0)
    {
       errLogRet(ErrLogOp,debugInfo,
		"sendCodes: Exp: '%s' failed, Start Exp unable to be  sent. ExpState: %d\n",
		 ActiveExpInfo.ExpId,ActiveExpInfo.ExpInfo->ExpState);
       mapOutExp(&ActiveExpInfo);
       return(-1);
       /* Need some type of msge back to Expproc to indicate failure */
    }
    DPRINT1(1,"Send RT Params. File: '%s'\n",ActiveExpInfo.ExpInfo->RTParmFile);
    if ( ((int) strlen(ActiveExpInfo.ExpInfo->RTParmFile) > 0) )
    {
       DPRINT(1,"Send RT Params.\n");
       result = sendRTFile(ActiveExpInfo.ExpInfo->RTParmFile,
			ActiveExpInfo.ExpInfo->AcqBaseBufName);
			/* bufRootName); */
       if (result < 0)
       {
          sendExpStat("endExp");
          errLogRet(ErrLogOp,debugInfo,
		"sendCodes: Exp: '%s' failed, RT File unable to be  sent. ExpState: %d\n",
		 ActiveExpInfo.ExpId,ActiveExpInfo.ExpInfo->ExpState);
          mapOutExp(&ActiveExpInfo);
          return(-1);
        /* Need some type of msge back to Expproc to indicate failure */
       }
    }

    DPRINT1(1,"Send Tables. File: '%s'\n",ActiveExpInfo.ExpInfo->TableFile);
    if ( ((int) strlen(ActiveExpInfo.ExpInfo->TableFile) > 0) )
    {
      /* result = sendTblFile(ActiveExpInfo.ExpInfo->NumTables,
			ActiveExpInfo.ExpInfo->TableFile,
			ActiveExpInfo.ExpInfo->AcqBaseBufName); */
       DPRINT(1,"Send Tables.\n");
       result = sendTblFiles(ActiveExpInfo.ExpInfo->NumTables,
			ActiveExpInfo.ExpInfo->TableFile,
			ActiveExpInfo.ExpInfo->AcqBaseBufName);
       if (result < 0)
       {
          sendExpStat("endExp");
          errLogRet(ErrLogOp,debugInfo,
		"sendCodes: Exp: '%s' failed, Table File unable to be sent. ExpState: %d\n",
		 ActiveExpInfo.ExpId,ActiveExpInfo.ExpInfo->ExpState);
          mapOutExp(&ActiveExpInfo);
          return(-1);
        /* Need some type of msge back to Expproc to indicate failure */
       }
    }

    DPRINT1(1,"Send WForms. File: '%s'\n",ActiveExpInfo.ExpInfo->WaveFormFile);
    if ( ((int) strlen(ActiveExpInfo.ExpInfo->WaveFormFile) > 0) )
    {
       DPRINT(1,"Send Waveforms.\n");
       result = sendWFFile(ActiveExpInfo.ExpInfo->WaveFormFile,
			ActiveExpInfo.ExpInfo->AcqBaseBufName);
			/* bufRootName); */
       if (result < 0)
       {
          sendExpStat("endExp");
          errLogRet(ErrLogOp,debugInfo,
		"sendCodes: Exp: '%s' failed, Waveform File unable to be sent. ExpState: %d\n",
		 ActiveExpInfo.ExpId, ActiveExpInfo.ExpInfo->ExpState);
          mapOutExp(&ActiveExpInfo);
          return(-1);
        /* If the waveform file is not found that may not be an error */
       }
    }

    DPRINT1(1,"SEND Acodes, File: '%s'\n",ActiveExpInfo.ExpInfo->Codefile);
    if ( ((int) strlen(ActiveExpInfo.ExpInfo->Codefile) > 0) )
    {
       DPRINT(1,"SEND Acodes.\n");
       result = sendAcodes(ActiveExpInfo.ExpInfo->NumAcodes,
			ActiveExpInfo.ExpInfo->Codefile,
			ActiveExpInfo.ExpInfo->AcqBaseBufName);
       if ( result == -99)
       {
	  DPRINT1(1,"Acode Transfer was Aborted, ExpState: %d\n", 
		   ActiveExpInfo.ExpInfo->ExpState);
       }
       if (result < 0 && result != -99)
       {
          sendExpStat("endExp");
          errLogRet(ErrLogOp,debugInfo,
		"sendCodes: Exp: '%s' failed, Acode File unable to be sent. ExpState: %d\n",
		 ActiveExpInfo.ExpId, ActiveExpInfo.ExpInfo->ExpState);
          mapOutExp(&ActiveExpInfo);
          return(-1);
        /* Need some type of msge back to Expproc to indicate failure */
       }
    }

    mapOutExp(&ActiveExpInfo);

    AbortXfer = 0;
    return(0);
}

int sendExpStat(char *str)
{
   char cmdstr[256];
  /*  Format:Cmd BufType Label(base) Size(max) number start_number  */

  sprintf(cmdstr,"%s none none 0 1 0",str);
  blockAbortTransfer();
  sendCommand(cmdstr,DLCMD_SIZE,response,DLRESPB);
  unblockAbortTransfer();
  if (strncmp(response,"OK",2) != 0)
  {
     errLogRet(LOGOPT,debugInfo,
		"sendExpStat: Console Transfer Error: '%s'\n",cmdstr);
     return(-1);
  }
  return(0);
}

int sendRTFile(char *filename,char *bufRootName)
{
   MFILE_ID ifile;
   char cmdstr[256];
   int size,bytes,stat;
  /*  Format:Cmd BufType Label(base) Size(max) number start_number  */

  stat = 0;
  /* Forget Malloc and Read, we'll use the POWER of MMAP */
  ifile = mOpen(filename,0,O_RDONLY);
  if (ifile == NULL)
  {
    errLogSysRet(LOGOPT,debugInfo,"could not open %s",filename);
    return(-1);
  }
  if (ifile->byteLen == 0)
  {
    errLogRet(LOGOPT,debugInfo,"sendRTFile: '%s' is Empty\n",filename);
    mClose(ifile);
    return(-1);
  }
  /* going to read sequential once */
  mAdvise(ifile,MF_SEQUENTIAL);

  size = ifile->byteLen;


  blockAbortTransfer();

  sprintf(cmdstr,"downLoad Dynamic %srtv %d 1 0",bufRootName,size);
  DPRINT1(1,"Send: %s\n",cmdstr);
  sendCommand(cmdstr,DLCMD_SIZE,response,DLRESPB);
  DPRINT1(1,"Respond: %s\n",response);
  if (strncmp(response,"OK",2) == 0)
  {
    bytes = writeToConsole(chanId,ifile->offsetAddr,size);
    if (bytes != size)
    {
     errLogRet(LOGOPT,debugInfo,
		"sendRTFile: Of %d bytes only %d xfr to console\n",size,bytes);
     stat = -1;
    }
  }
  else
  {
     errLogRet(LOGOPT,debugInfo,
		"sendRTFile: Console Transfer Error: '%s'\n",cmdstr);
     stat = -1;
  }

  unblockAbortTransfer();

  mClose(ifile);

  return(stat);
}


/* send all the tables in a more efficient manner  */
int sendTblFiles(unsigned int nTables, char *filename,char *bufRootName)
{
   MFILE_ID ifile;
   CODELIST CodeList;
   char cmdstr[256];
   int k,size;
   int bytes __attribute__((unused));
   int codesize,maxSize;
   int maxAvailable;
   unsigned int *offsetIndx;

  /*  Format:Cmd BufType Label(base) Size(max) number start_number  */

  /* Forget Malloc and Read, we'll use the POWER of MMAP */
  ifile = mOpen(filename,0,O_RDONLY);
  if (ifile == NULL)
  {
    errLogSysRet(LOGOPT,debugInfo,"could not open %s",filename);
    return(-1);
  }
  if (ifile->byteLen == 0)
  {
    errLogRet(LOGOPT,debugInfo,"sendTblFile: '%s' is Empty\n",filename);
    mClose(ifile);
    return(-1);
  }
  /* going to read sequential once */
  /* mAdvise(ifile,MF_SEQUENTIAL); */

   /* number of acodes in file */
   /* nTables = (unsigned int) ifile->offsetAddr; */
   offsetIndx = (unsigned int*) (ifile->offsetAddr + sizeof(unsigned int));

   CodeList = (CODELIST) malloc(sizeof(CODEENTRY) * nTables);
   

   /* find largest Table set */
   for( k = 0, maxSize = 0; k < nTables - 1; k++)
   {
     codesize = ( offsetIndx[k+1] - offsetIndx[k] );
     CodeList[k].cSize = codesize;
     CodeList[k].pCode = ifile->offsetAddr + offsetIndx[k];
     maxSize = (codesize > maxSize) ? codesize : maxSize;
   }

   size = ifile->byteLen;
   codesize = ((unsigned int)size) - offsetIndx[nTables - 1];
   CodeList[k].cSize = codesize;
   CodeList[k].pCode = ifile->offsetAddr + offsetIndx[nTables - 1];
   maxSize = (codesize > maxSize) ? codesize : maxSize;
  

   /* downLoad Tables Dynamic basename numberOfTables */
   /* cmd,buftype,label,&size,&number,&startn */
   sprintf(cmdstr,"downLoad Tables %s %d %d %d",
	     bufRootName,maxSize,nTables,0);
   DPRINT1(1,"Send Cmdstr: '%s'\n",cmdstr);
   blockAbortTransfer();
   sendCommand(cmdstr,DLCMD_SIZE,response,DLRESPB);
   DPRINT1(1,"Responce: '%s'\n",response);
   maxAvailable = atoi(&response[3]);
   DPRINT1(1,"max tables that can be down loaded: %d\n",maxAvailable);
   if (strncmp(response,"OK",2) != 0)
   {
      errLogRet(LOGOPT,debugInfo,
	"sendTblFiles: Console Transfer Error: '%s'\n",cmdstr);
      unblockAbortTransfer();
      free(CodeList);
      mClose(ifile);
      return(-1);   /* return, download no further */
   }
   if (nTables > maxAvailable)
   {
     errLogRet(LOGOPT,debugInfo,
	"sendTblFiles: number of Tables %d exceeded maximum allowed to download %d\n",nTables,maxAvailable);
      unblockAbortTransfer();
      free(CodeList);
      mClose(ifile);
      return(-1);   /* return, download no further */
   }

   DPRINT1(1,"Sendproc: number of  tables %d\n",nTables);
   /* now send down the tables */
   for (k = 0; k < nTables; k++)
   {
     int sizeToSend;

     DPRINT2(1,"Sendproc: send table %d, size %d\n",
	k,CodeList[k].cSize);
     if ( (ActiveExpInfo.ExpInfo->ExpState >= EXPSTATE_TERMINATE)  &&
		(k > 0) )
     {
        int zero = 0;
        DPRINT1(0,"sendTblFile: Terminate Transfer: %d\n",ActiveExpInfo.ExpInfo->ExpState);

        bytes = writeToConsole(chanId, (char *) &zero,sizeof(int));
        unblockAbortTransfer();
        free(CodeList);
        mClose(ifile);
	return(-1);
     }

     sizeToSend = htonl(CodeList[k].cSize);
     bytes = writeToConsole(chanId, (char *) &sizeToSend,sizeof(int));
     bytes = writeToConsole(chanId,CodeList[k].pCode,CodeList[k].cSize);
   }
   unblockAbortTransfer();
   free(CodeList);
   mClose(ifile);
   getResponce(response,DLRESPB);
   DPRINT1(1,"Ack = '%s'\n",response);
   if (strncmp(response,"OK",2) != 0)
   {
      errLogRet(LOGOPT,debugInfo,
	"sendTblFiles: Console Transfer Error: '%s'\n",response);
      return(-1);   /* return, download no further */
   }
   return(0);		/* anything less than zero and sendproc gives errors! */
}




int sendWFFile(char *filename,char *bufRootName)
{
   MFILE_ID ifile;
   char cmdstr[256];
   int size,bytes,stat;
  /*  Format:Cmd BufType Label(base) Size(max) number start_number  */

  stat = 0;
  /* Forget Malloc and Read, we'll use the POWER of MMAP */
  ifile = mOpen(filename,0,O_RDONLY);
  if (ifile == NULL)
  {
    errLogSysRet(LOGOPT,debugInfo,"sendWFFile: could not open %s",filename);
    return(-1);
  }
  if (ifile->byteLen == 0)
  {
    errLogRet(LOGOPT,debugInfo,"sendWFFile: '%s' is Empty\n",filename);
    mClose(ifile);
    return(-1);
  }
  /* going to read sequential once */
  mAdvise(ifile,MF_SEQUENTIAL);

  size = ifile->byteLen;

  sprintf(cmdstr,"downLoad Dynamic %swf %d 1 0",bufRootName,size);
  DPRINT1(1,"Send WForm: %s\n",cmdstr);
  blockAbortTransfer();
  sendCommand(cmdstr,DLCMD_SIZE,response,DLRESPB);
  DPRINT1(1,"Respond: %s\n",response);
  if (strncmp(response,"OK",2) == 0)
  {
    bytes = writeToConsole(chanId,ifile->offsetAddr,size);
    if (bytes != size)
    {
     errLogRet(LOGOPT,debugInfo,
		"sendAFile: Of %d bytes only %d xfr to console\n",size,bytes);
     unblockAbortTransfer();
     stat = -1;
    }
  }
  else
  {
     errLogRet(LOGOPT,debugInfo,
		"sendWFFile: Console Transfer Error: '%s'\n",cmdstr);
     unblockAbortTransfer();
     stat = -1;
  }

  unblockAbortTransfer();
  mClose(ifile);

  return(stat);
}

/***************************************************************
* sendTables - Send a named buffer (Global Table) to the console
*
*   Args.   1)	  Name of file containing table(s).
*	    2)	  Order in file of 1st table to download (starting from 0).
*	    3)	  Name to give 1st table (e.g. "exp1t0updt").
*	    4)	  Order of 2nd table to download.
*	    5)	  Name to give second table.
*	    ...
*	    2n)	  Order in file of nth table to send.
*	    2n+1) Name to give nth table
*/
int sendTables(char *argstr)
  /* NOTE:
   * The "argstr" argument has been processed with strtok()--resulting in
   * a null being substituted for the first delimiter.
   * We ignore the "argstr" parameter, and continue
   * processing with strtok(NULL, ...).  Of course, this will fail
   * miserably if the parser is changed so that it no longer uses strtok(),
   * or if another call to strtok() occurs between these two.
   */
{
    char *filename;
    int rtn;
    char *sn;
    char *tblname;

    if ((filename = strtok(NULL," ")) == NULL){
        errLogRet(LOGOPT,debugInfo,
		"sendTables: No table file specified\n");
	return -1;
    }

    /* Send all the tables specified in the arg list */
    while ((sn=strtok(NULL, " ")) && (tblname=strtok(NULL," "))){
	if ( (rtn=sendTable(filename, atoi(sn), tblname)) ) {
	    /* If error, send no more tables */
	    break;
	}
    }

    return rtn;
}

/***************************************************************
* sendDelTables - Send and delete a named buffer (Global Table) to the console
*
*   Args.   1)	  Name of file containing table(s).
*	    2)	  Order in file of 1st table to download (starting from 0).
*	    3)	  Name to give 1st table (e.g. "exp1t0updt").
*	    4)	  Order of 2nd table to download.
*	    5)	  Name to give second table.
*	    ...
*	    2n)	  Order in file of nth table to send.
*	    2n+1) Name to give nth table
*/
int
sendDelTables(char *argstr)
  /* NOTE:
   * The "argstr" argument has been processed with strtok()--resulting in
   * a null being substituted for the first delimiter.
   * We ignore the "argstr" parameter, and continue
   * processing with strtok(NULL, ...).  Of course, this will fail
   * miserably if the parser is changed so that it no longer uses strtok(),
   * or if another call to strtok() occurs between these two.
   */
{
    char *filename;
    int rtn;
    char *sn;
    char *tblname;

    if ((filename = strtok(NULL," ")) == NULL){
        errLogRet(LOGOPT,debugInfo,
		"sendTables: No table file specified\n");
	return -1;
    }

    /* Send all the tables specified in the arg list */
    while ((sn=strtok(NULL, " ")) && (tblname=strtok(NULL," "))){
	if ( (rtn=sendTable(filename, atoi(sn), tblname)) ) {
	    /* If error, send no more tables */
	    break;
	}
    }
    unlink(filename);
    return rtn;
}

// sendTable(char *fname,		/* Name of table file */
// 	  int tab_nbr,		/* Position in file of table to download */
// 	  char *tname)		/* Name to give the downloaded table */
int sendTable(char *fname, int tab_nbr, char *tname)
{
    char cmdstr[256];
    MFILE_ID ifile;
    int *lptr;
    int ntbls;
    char *ptbl;
    int tablelen;
    int tableoffset;

    /* Open the file */
    ifile = mOpen(fname, 0, O_RDONLY);
    if (ifile == NULL){
	errLogSysRet(LOGOPT, debugInfo, "sendTable: cannot open %s", fname);
	return -1;
    }
    if (ifile->byteLen == 0){
	errLogRet(LOGOPT, debugInfo, "sendTable: '%s' is empty\n", fname);
	mClose(ifile);
	return -1;
    }

    /* Go to nth table */
    lptr = (int *)(ifile->offsetAddr);
    ntbls = *lptr;		/* Nbr of tables in file */
    if (tab_nbr >= ntbls){
	/* (First table is tab_nbr=0) */
	errLogRet(LOGOPT, debugInfo,
		  "sendTable: want table %d (1st is 0); %d tables in %s\n",
		  tab_nbr, ntbls, fname);
	mClose(ifile);
	return -1;
    }
    tableoffset = lptr[tab_nbr+1]; /* Byte offset of beginning of the table */
    if (tab_nbr+1 < ntbls){
	tablelen = lptr[tab_nbr+2] - tableoffset;
    }else{
	tablelen = ifile->byteLen - tableoffset;
    }
    ptbl = ifile->offsetAddr + tableoffset;

    /*offsetIndx = (unsigned int*) (ifile->offsetAddr + sizeof(unsigned int));
    CodeList = (CODELIST) malloc(sizeof(CODEENTRY) * nTables);*/

    /* Download it with specified name */
    sprintf(cmdstr,"downLoad Dynamic %s %d %d %d", tname, tablelen, 1, tab_nbr);
    blockAbortTransfer();
    sendCommand(cmdstr, DLCMD_SIZE, response, DLRESPB);
    if (strncmp(response, "OK", 2) == 0){
	writeToConsole(chanId, ptbl, tablelen);
    }else{
        errLogRet(LOGOPT, debugInfo,
		  "sendTable: Console Transfer Error: '%s'\n", cmdstr);
        unblockAbortTransfer();
	mClose(ifile);
        return -1;
    }
   unblockAbortTransfer();
    mClose(ifile);

    return 0;
}

int
sendCmd(char *argstr)
  /*
   * argstr = "command<NUL>[XParseCmd | AParseCmd | AUpdtCmd] ...args..."
   * e.g.: "command AUpdtCmd 1; %d, 0;", where %d is the number CHG_TABLE
   *
   * NOTE:
   * The "argstr" argument has been processed with strtok()--resulting in
   * a null being substituted for the first delimiter.
   * We ignore the "argstr" parameter, and continue
   * processing with strtok(NULL, ...).  Of course, this will fail
   * miserably if the parser is changed so that it no longer uses strtok(),
   * or if another call to strtok() occurs between these two.
   */
{
    char *args;

    args = strtok(NULL, "");
    sendCommand(args, DLCMD_SIZE, response, DLRESPB);
    return 0;
}


/****************************************************/
/****************************************************/
/****************************************************/
/****************************************************/

#define NCBFX	  200	/* more than psg's 32 */
struct _compr_
{
   unsigned char *ucpntr;
   int size;
} ref_table[NCBFX];

static void
init_compressor()
{
  int i;
  for (i=0; i < NCBFX; i++)
  {
    ref_table[i].ucpntr = NULL;
    ref_table[i].size   = 0;
  }
}

int decompress(unsigned char *dest, unsigned char *src,
               unsigned char *ref, int num)
{
   int xcnt;
   int k;
   xcnt=0;
   while (num-- > 0)
   {
     /* all is keyed off 0 in stream */
     if (*src != 0)
     {
       *dest++ =   *ref++ ^ *src++;  /* uncompressed case */
       xcnt++;
     }
     else 
     {
       src++;  /* 2 count field */
       k = *src++; /* src is next */
       num--;
       while (k-- > 0)
       {
         *dest++ = *ref++;
         xcnt++;
       }
     }
   }
   return(xcnt);
}
      
void install_ref(int size,int bufn,unsigned char *ptr)
{
   /* test is programming error test */
   if (ref_table[bufn].size != 0) 
   {
      if (ref_table[bufn].size != size)
	fprintf(stderr,"size mismatch %d in table %d in this buf\n",
	   ref_table[bufn].size,size);
      return;  /*  CAREFUL */
   }
   ref_table[bufn].ucpntr = ptr;
   ref_table[bufn].size = size;
}

unsigned char *get_ref(int num)
{
   if ((num < 0) || (num >= NCBFX))
   {
     fprintf(stderr,"no such reference %d\n",num);
     return(NULL);
   }
   return(ref_table[num].ucpntr);
}

#define NO_CODE    0
#define REF_CODE  64
#define CMP_CODE 128

/*---------------------------------------------------------------------
+--------------------------------------------------------------------*/
unsigned char *
send_code(unsigned char *addr, unsigned char *tmpptr, int codesize, int buf_num)
{
    int index_x;
    unsigned char *codes;
    int action;
    int full_size;
    unsigned char *pp1;

    /***************************************************/
    /*  sizes 					       */
    /***************************************************/

    action= 0x0c0 & buf_num;
    full_size = ((unsigned int) (0xffffff00 & buf_num)) >> 8;
    buf_num  &= 0x03f; 
    codes = addr;
    DPRINT3(1,"send_code(): action= %d full_size= %d buffer= %d\n",
                    action,full_size,buf_num);
    /*******************************************
    ** decipher action 
    ** 
    *******************************************/
    switch (action) 
    {
	case REF_CODE: install_ref(full_size,buf_num,addr); 
        case NO_CODE:   break;
	case CMP_CODE: 
	 pp1 = get_ref(buf_num);
	 if (pp1 == NULL)
	 {
	   /* see if you can look it up!! */
	    fprintf(stderr,"readexpcode(): no reference acode\n");
	   return(0);
	 }
	 index_x=decompress(tmpptr,addr,pp1,codesize);

	 if (index_x != full_size) 
	 {
	    fprintf(stderr,"readexpcode(): decompress %d ,expected  %d \n",
                    index_x,full_size);
	    perror("READEXPCODE decompress mismatch\n");
	    exit(-1);
	 }
	 codes = tmpptr;
	 break;
     }
    return(codes);
}
static void
get_code_info(unsigned char *addr, unsigned char *out)
{
   int i;

   for (i=0; i < 2 * sizeof(int); i++)
      *out++ = *addr++;
}

/****************************************************/
/****************************************************/
/****************************************************/
/****************************************************/
int sendAcodes(int nCodes, char *filename,char *bufRootName)
{
   char cmdstr[256],*tmp;
   int k,bytes,stat,prtcnt;
   int codesize,maxSize;
   int acqBufsFree;
   short *bigger = NULL;
   int  *statusBlk = NULL;
   int scratch_size = 0;
   int codeinfo[2];
   int ntimes, nTimes2sendAcodes, startAcode;
   int abortSent;
   int nFIDs;	/* number of Fids or ArrayDim */
   int JpsgFlag; 

   abortSent = stat = 0;
  /*  Format:Cmd BufType Label(base) Size(max) number start_number  */

  /* Forget Malloc and Read, we'll use the POWER of MMAP */
  if ( ActiveExpInfo.ExpInfo->ExpState >= EXPSTATE_TERMINATE)
  {
    return(-1);
  }
  mapAcodes = mOpen(filename,0,O_RDONLY);
  if (mapAcodes == NULL)
  {
    errLogSysRet(LOGOPT,debugInfo,"could not open %s",filename);
    return(-1);
  }
  if (mapAcodes->byteLen == 0)
  {
    errLogRet(LOGOPT,debugInfo,"sendAFile: '%s' is Empty\n",filename);
    mClose(mapAcodes);
    mapAcodes = NULL;
    return(-1);
  }
  /* going to read sequential once */
  /* mAdvise(mapAcodes,MF_SEQUENTIAL); */

   /* number of acodes in file */
   /* nAcodes = *((int*) mapAcodes->offsetAddr); */
   nAcodes = nCodes;
   nFIDs = ActiveExpInfo.ExpInfo->ArrayDim;	/* Total Elements of this Exp. */
   startAcode = ActiveExpInfo.ExpInfo->CurrentElem;
   maxSize = ActiveExpInfo.ExpInfo->acode_max_size;
   scratch_size = maxSize + 2048;
   bigger = (short *) malloc(scratch_size);
   statusBlk = (int*) bigger;

   /*
   // Used for RA, if Jpsg then nAcodes will be 1, even if nFIDs is greater than 1
   // If nFIDs is one then Std PSG & Jpsg are equivent in sending Acodes to console
   */
   /* JpsgFlag = ((nAcodes == 1) && (nFIDs > 1)) ? 1 : 0; */
   JpsgFlag = (ActiveExpInfo.ExpInfo->PSGident == 100) ? 1 : 0;
   DPRINT4(1,"sendAcodes: # Acodes: %d, # FIDs: %d,  Jpsg flag: %d, startAcode: %d\n",
	nAcodes,nFIDs,JpsgFlag,startAcode);

   if (JpsgFlag == 1)
      startAcode = 0;
   DPRINT1(1,"New startAcode value: %d\n",startAcode);

   init_compressor();

   codesize = 0;

   // size = mapAcodes->byteLen;

   /* Since the console thinks all codes are the same size, and we told
      it the max size then we must write the max size to it.
      Since this is a mmap file the last code has the possibility
      of giving a Bus Error if the write goes past the mmap pages
      of the file. Most of the time no problem. To be sure we do
      the following code. 

      PSG now writes extra stuff of the acode file to avoid this problem
   */
   strcpy(labelBase,bufRootName);

   /* nAcodes + 1 (extra one for the stat block sent down for abort or complete */
   sprintf(cmdstr,"downLoad Fixed %s %d %d %d",bufRootName,maxSize,
						nAcodes+1,startAcode);
   DPRINT1(1,"Send: %s\n",cmdstr);

   blockAbortTransfer();

     sendCommand(cmdstr,DLCMD_SIZE,response,DLRESPB);

   unblockAbortTransfer();

   DPRINT1(1,"Respond: %s\n",response);
   if (strncmp(response,"OK",2) == 0)
   {
     char *code_ptr;

     tmp = strtok(response," ");  /* "OK" */
     tmp = strtok(NULL," ");  /* number of free buffers */
     acqBufsFree = atol(tmp);

     /* -------- Interleaving -----------------------*/
     /* Interleaving & can't fix all acodes down in console */
     /* For Interleave, we take the simple approach whereby we just
        send all the acodes down mulitple times.
	Number of time equals NT / BS, e.g. nt=10, bs=5 10/5 = 2 times
     */
     if ( (ActiveExpInfo.ExpInfo->IlFlag != 0) && (nCodes > 1) )
					/* && (acqBufsFree < nCodes)) */
     {
	nTimes2sendAcodes = 
	 (ActiveExpInfo.ExpInfo->NumTrans
		- ActiveExpInfo.ExpInfo->CurrentTran) /
					ActiveExpInfo.ExpInfo->NumInBS;
	if (((ActiveExpInfo.ExpInfo->NumTrans 
		- ActiveExpInfo.ExpInfo->CurrentTran) % 
			ActiveExpInfo.ExpInfo->NumInBS) > 0)
	    nTimes2sendAcodes++;
		
        DPRINT1(1,"Interleave, Times to send Acode set: %d\n",nTimes2sendAcodes);
     }
     else
     {
	nTimes2sendAcodes = 1;
        DPRINT1(1,"Non-Interleave, Times to send Acode set: %d\n",nTimes2sendAcodes);
     }
     
     /* For Non-Interleaved Exp nTimes2sendAcodes = 1,
        For Interleave Exp we will send donw the set of Acodes mulitple times
     */
     for ( ntimes = 0; ntimes < nTimes2sendAcodes; ntimes++)
     {

        DPRINT1(1,"%d Set of Acodes being sent\n",ntimes+1);
        /* for each new set after the 1st, we must tell downLoader that the Acodes
	   are comming.
        */
        /* k = ActiveExpInfo.ExpInfo->FidsCmplt; */
        prtcnt = 0;
        codesize = 0;
   	DPRINT1(1,"Sending %d Fids\n",nAcodes);
        bytes = 0;
        for (k = 0; k < nAcodes; k++)
        {
   	   DPRINT1(1,"Sending Acode: %d\n",k);
	   get_code_info((unsigned char *) (mapAcodes->offsetAddr + codesize),
			 (unsigned char *) &codeinfo[0]);
           DPRINT2(2,"got code info %d %d, ",codeinfo[0], codeinfo[1]);
           codesize += 2 * sizeof(int);
           code_ptr = (char*) send_code((unsigned char *) (mapAcodes->offsetAddr + codesize), 
				(unsigned char *) bigger,
                     		codeinfo[0],codeinfo[1]);
           /* if ( (AbortXfer != 0) && (k > 0) ) */
    	   if ( (ActiveExpInfo.ExpInfo->ExpState >= EXPSTATE_TERMINATE) &&
		(k > 0) )
           {
	      DPRINT(1,"Acode Transfer Aborted\n");
              if ( (bytes != maxSize) && (bytes != -1) ) /* transfer was interrupt by abort */
              {
                DPRINT3(1,"Transfer Incomplete: of %d only %d bytes sent, send additional %d bytes: ",
				maxSize,bytes,maxSize-bytes);

  	 	blockAbortTransfer();

                writeToConsole(chanId,code_ptr,maxSize - bytes);

   		unblockAbortTransfer();
              }
	      statusBlk[0] = htonl(XFER_STATUS_BLK);
	      statusBlk[1] = htonl(XFER_ABORTED);

  	      blockAbortTransfer();

              bytes = writeToConsole(chanId, (char *) statusBlk,maxSize);

   	      unblockAbortTransfer();

   	      DPRINT2(1,"On Fid %d, Xfer Aborted: bytes: %d\n",k,bytes);
	      abortSent = 1;
     	      stat = -99;
	      return(-99);
	   }
	   if (k >= startAcode)
	   {

  	      blockAbortTransfer();

              bytes = writeToConsole(chanId,code_ptr,maxSize);

   	      unblockAbortTransfer();

   	      DPRINT3(1,"Sent Fid: %d, bytes: %d, (%d)\n",k,bytes,maxSize);
              if (bytes <= 0)
              {
		DPRINT1(1,"Transfer Warning: %s\n", strerror( errno ) );
	      }
              if (bytes != maxSize)
              {
                DPRINT2(0,
		"Acode Transfer Incomplete: of %d only %d bytes sent: ",
			maxSize,bytes);
	      }
   	      DPRINT2(2,"a. Sent Fid: %d, bytes: %d\n",k,bytes);
              ActiveExpInfo.ExpInfo->LastCodeSent = k;
	   }
           codesize += codeinfo[0];
           if ( ++prtcnt >= 9)
           {
	      DPRINT(2,"\n");
	      prtcnt = 0;
           }
        }
        if (nTimes2sendAcodes > 1)
        {
	  statusBlk[0] = htonl(XFER_STATUS_BLK);
	  statusBlk[1] = htonl(XFER_RESETCNT);

  	  blockAbortTransfer();

          bytes = writeToConsole(chanId, (char *) statusBlk,maxSize);

   	  unblockAbortTransfer();

   	  DPRINT(1,"Xfer Reset Count, so interleave cycle start with right buffer name\n");
        }
	/* Interleaving: When done with the last acode set start with	*/
	/* 		 the first acode set until nTimes2sendAcodes.	*/
        if (ActiveExpInfo.ExpInfo->IlFlag != 0)
	  startAcode = 0;
     }
     DPRINT2(1,"AbortXfer: %d, abortSent: %d \n",AbortXfer,abortSent);
     if ( abortSent == 0 ) 
     {
        DPRINT(1,"Acode Transfer Complete\n");
        statusBlk[0] = htonl(XFER_STATUS_BLK);
        statusBlk[1] = htonl(XFER_COMPLETE);

  	blockAbortTransfer();

        bytes = writeToConsole(chanId, (char *) statusBlk,maxSize);

   	unblockAbortTransfer();

   	DPRINT2(1,"On Fid %d, Xfer Complete: bytes: %d\n",k,bytes);
     }
   }
   else
   {
     errLogRet(LOGOPT,debugInfo,
		"sendAcodes: Console Transfer Error: '%s'\n",cmdstr);
     stat = -1;
   }
  /* Not interleaved and all codes sent down then we are done,
     otherwise maybe not so don't close files
  */
  free(bigger);

  mClose(mapAcodes);
  mapAcodes = NULL;

  return(stat);
}

int sendCommand(char *command,int trsize,char *response,int recvsiz)
{
   int trouble;
   blockAllEvents();
   trouble = writeChannel(chanId,command,trsize);
   unblockAllEvents();
   if (trouble != trsize)
   {
      errLogRet(LOGOPT,debugInfo,
	"sendCommand: writeChannel() wrote %d should be %d bytes\n",
	  trouble,trsize);
     return(-1);
   }
   blockAllEvents();
   trouble = readChannel(chanId,response,recvsiz);
   unblockAllEvents();
   if (trouble != recvsiz)
   {
      errLogRet(LOGOPT,debugInfo,
	"sendCommand: readChannel() read %d should be %d bytes\n",
	  trouble,recvsiz);
     return(-1);
   }
   return(0);
}

int getResponce(char *response,int recvsiz)
{
   int trouble;
   blockAllEvents();
   trouble = readChannel(chanId,response,recvsiz);
   unblockAllEvents();
   if (trouble != recvsiz)
   {
      errLogRet(LOGOPT,debugInfo,
	"getResponce: readChannel() read %d should be %d bytes\n",
	  trouble,recvsiz);
     return(-1);
   }
   return(0);
}

int writeToConsole(int chanId,char* bufAdr,int size)
{
  int i,tot_size,nblocks,nremain,tbytes;

  tbytes = 0;
  tot_size = size;

  // bufsize = DLXFR_SIZE;
  // if (tot_size < DLXFR_SIZE)
  //   bufsize = tot_size;

  nblocks = tot_size / DLXFR_SIZE;
  nremain = tot_size % DLXFR_SIZE;
    
  for (i = 0; i < nblocks; i++)
  {
   blockAllEvents();
     tbytes += writeChannel(chanId,bufAdr,DLXFR_SIZE);
   unblockAllEvents();
     bufAdr += DLXFR_SIZE;
  }
  /* we should now be in the last bit */
  if (nremain > 0)
  {
   blockAllEvents();
    tbytes += writeChannel(chanId,bufAdr,nremain);
   unblockAllEvents();
  }
  return(tbytes);
}

int abortCodes(char *str)
{
   DPRINT(1,"abortCodes: \n");
   return(0);
}

int terminate(char *str)
{
   extern int shutdownMsgQ();
   DPRINT(1,"terminate: \n");
   shutdownComm();
   exit(0);
}

void ShutDownProc()
{
   shutdownComm();
   resetState();
}

void resetState()
{
  labelBase[0] = '\0';
  nAcodes = 0;
  if (mapAcodes != NULL)
  {
     mClose(mapAcodes);
  }
  if ( CodeList != NULL)
  {
     free(CodeList);
  }
  if ((int) strlen(ActiveExpInfo.ExpId) > 1)
  {
    mapOutExp(&ActiveExpInfo);
  }
}

int debugLevel(char *str)
{
    extern int DebugLevel;
    char *value;
    int  val;
    value = strtok(NULL," ");
    val = atoi(value);
    DPRINT1(1,"debugLevel: New DebugLevel: %d\n",val);
    DebugLevel = val;
    return(0);
}


int mapInExp(ExpEntryInfo *expid)
{
    DPRINT1(2,"mapInExp: map Shared Memory Segment: '%s'\n",expid->ExpId);

    expid->ShrExpInfo = shrmCreate(expid->ExpId,SHR_EXP_INFO_RW_KEY,(unsigned int)sizeof(SHR_EXP_STRUCT)); 
    if (expid->ShrExpInfo == NULL)
    {
       errLogSysRet(LOGOPT,debugInfo,"mapInExp: shrmCreate() failed:");
       return(-1);
    }
    if (expid->ShrExpInfo->shrmem->byteLen < sizeof(SHR_EXP_STRUCT))
    {
        /* hey, this file is not a shared Exp Info file */
       shrmRelease(expid->ShrExpInfo);         /* release shared Memory */
       unlink(expid->ExpId);        /* remove filename that shared Mem created */
       expid->ShrExpInfo = NULL;
       return(-1);
    }


#ifdef DEBUG
    if (DebugLevel >= 3)
       shrmShow(expid->ShrExpInfo);
#endif

    expid->ExpInfo = (SHR_EXP_INFO) shrmAddr(expid->ShrExpInfo);

    /* Should open the shared Exp status shared structure here */

    return(0);
}

int mapOutExp(ExpEntryInfo *expid)
{
    DPRINT1(2,"mapOutExp: unmap Shared Memory Segment: '%s'\n",expid->ExpId);

    if (expid->ShrExpInfo != NULL)
      shrmRelease(expid->ShrExpInfo); 
    
    if (expid->ShrExpStatus != NULL)
      shrmRelease(expid->ShrExpStatus); 

    memset((char*) expid,0,sizeof(ExpEntryInfo));  /* clear struct */

    return(0);
}

int mapIn(char *str)
{
    char* filename;

    filename = strtok(NULL," ");

    DPRINT1(1,"mapIn: map Shared Memory Segment: '%s'\n",filename);

    ShrExpInfo = shrmCreate(filename,SHR_EXP_INFO_RW_KEY,(unsigned int)sizeof(SHR_EXP_STRUCT)); 
    if (ShrExpInfo == NULL)
    {
       errLogSysRet(LOGOPT,debugInfo,"mapIn: shrmCreate() failed:");
       return(-1);
    }

#ifdef DEBUG
    if (DebugLevel >= 3)
      shrmShow(ShrExpInfo);
#endif

    expInfo = (SHR_EXP_INFO) shrmAddr(ShrExpInfo);

    return(0);
}

int mapOut(char *str)
{
    char* filename;

    filename = strtok(NULL," ");

    DPRINT1(1,"mapOut: unmap Shared Memory Segment: '%s'\n",filename);

    shrmRelease(ShrExpInfo);
    ShrExpInfo = NULL;
    expInfo = NULL;
    return(0);
}


#ifndef USESIGUSR2_4_ABORT

/* dummy the SIGUSR2 routines, now use shrexpinfo.h ExpState to determine aborts, etc. */

int blockAbortTransfer() { return(0); }

int unblockAbortTransfer() { return(0); }

int setupAbortXfer() { return(0); }

#else

void abortTransfer()
{
   DPRINT(1,"abortTransfer() SIGUSR2 Handler: Abort Transfer \n");
   AbortXfer = 1;
   return;
}

int blockAbortTransfer()
{
   sigset_t		qmask;

   sigemptyset( &qmask );
   sigaddset( &qmask, SIGUSR2 );
   sigprocmask( SIG_BLOCK, &qmask, NULL );
   return(0);
}
int unblockAbortTransfer()
{
   sigset_t		qmask;

   sigemptyset( &qmask );
   sigaddset( &qmask, SIGUSR2 );
   sigprocmask( SIG_UNBLOCK, &qmask, NULL );
   return(0);
}
/*-------------------------------------------------------------------------
|
|   Setup the exception handlers for aborting transfer SIGUSR1 
|    
+--------------------------------------------------------------------------*/
void setupAbortXfer()
{
    sigset_t		qmask;
    struct sigaction	intquit;
    struct sigaction	segquit;

    /* --- set up interrupt handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGINT );
    sigaddset( &qmask, SIGALRM );
    sigaddset( &qmask, SIGIO );
    sigaddset( &qmask, SIGCHLD );
    sigaddset( &qmask, SIGQUIT );
    sigaddset( &qmask, SIGPIPE );
    sigaddset( &qmask, SIGALRM );
    sigaddset( &qmask, SIGTERM );
    sigaddset( &qmask, SIGUSR1 );

    intquit.sa_handler = abortTransfer;
    intquit.sa_mask = qmask;
/*     intquit.sa_flags = SA_RESTART; */

    intquit.sa_flags = 0;
    sigaction(SIGUSR2,&intquit,0L);
}

#endif

/*  Facility for downloading programs from the host to
    an arbitrary address on the VME bus.

    This was to be used to download DSP at system bootup, but it does not
    work reliably.

    Sometimes the Sendproc received the download DSP message before it had
    completed its connection to the downlinker in the console.  When this
    happened the initial writeCommand failed.  We tried requeueing the message
    through the Sendproc message queue, but this only served to put the
    Sendproc in an arbitrary loop.  For some reason the message from the
    console to complete the Sendproc-downlinker connection never got through.

    Ideally the Sendproc needs to defer messages, or at least "vme" messages
    until its connection to the console is complete; or the console needs to
    wait to send its Console Information Block until the Sendproc-downlinker
    connection is complete.  Absent either of these the download to the DSP
    board is handled by Expproc; its channel is ready when it receives the
    Console Information Block.

    This program is ready to be used and no modifications should be required,
    with the possible exception of the block executed if consoleConn == 0.	*/

int sendVME(char *argstr )
{
    char *downLoadFile, *memaddr, *vmeAddrAscii;
    char cmdstr[256];
    int dwnldrem, xfercount, xfersize;
    MFILE_ID md;

    downLoadFile = strtok( NULL, " " );
    if (downLoadFile != NULL)
      vmeAddrAscii = strtok( NULL, " " );
    if (downLoadFile == NULL || vmeAddrAscii == NULL)
    {
        errLogRet(LOGOPT,debugInfo,"sendVME: file to download or VME address not included\n");
	/* deliverMessage( "Expproc", "dwnldComplete  \nBad Parameters" ); */
	deliverMessage( "Expproc", "dwnldComplete  \nBad Parameters\n0x00eeeeee\n");
        return( -1 );
    }

    if (consoleConn() == 0) {
	errLogRet( ErrLogOp, debugInfo, "Sendproc can't download VME, console not connected\n" );
	/* deliverMessage( "Expproc", "dwnldComplete  \nNot Ready" ); */
	sprintf(dspdlstat,"dwnldComplete  \nNot Ready\n%s\n",vmeAddrAscii);
	deliverMessage( "Expproc", dspdlstat);
        return( -1 );
    }

    md = mOpen( downLoadFile, (uint64_t)0, O_RDONLY );
    if (md == NULL)
    {
        errLogRet( ErrLogOp, debugInfo,
	   "can't access DSP download file %s\n", downLoadFile
        );
	/* deliverMessage( "Expproc", "dwnldComplete  \nNo File" ); */
	sprintf(dspdlstat,"dwnldComplete  \nNo File\n%s\n",vmeAddrAscii);
	deliverMessage( "Expproc", dspdlstat);
        return( -1 );
    }

#if 0
	errLogRet( ErrLogOp, debugInfo, "Sendproc download DSP:\n" );
	errLogRet( ErrLogOp, debugInfo, "Sendproc use file %s\n", downLoadFile );
	errLogRet( ErrLogOp, debugInfo, "Sendproc with size %lld\n", md->byteLen );
	errLogRet( ErrLogOp, debugInfo, "Sendproc use VME addr %s\n", vmeAddrAscii );
#endif

    /*
     * There is a race between the console setting up its communication sockets and sending
     * the DSP download. Without the following sleep(), the sendCommand() will sometimes
     * hang up trying to do a readChannel() from the console.
     */
    sleep(2);  
    sprintf( &cmdstr[ 0 ], "downLoad VME %s %ld", vmeAddrAscii, md->byteLen );
    sendCommand(&cmdstr[ 0 ],DLCMD_SIZE,&response[ 0 ],DLRESPB);
    /*errLogRet( ErrLogOp, debugInfo,"Respond: %s\n",response);*/
    if (strncmp( "OK", response, strlen( "OK" ) ) == 0)
    {
        memaddr = md->mapStrtAddr;
        dwnldrem = md->byteLen;
        while (dwnldrem > 0) {
            xfersize = (dwnldrem > XFR_SIZE) ? XFR_SIZE : dwnldrem;
            xfercount = writeChannel( chanId, memaddr, xfersize );
            if (xfercount < 1) {
                break;
            }
            memaddr += xfercount;
            dwnldrem = dwnldrem - xfercount;
        }

	if (dwnldrem < 1)
        {
	  /* deliverMessage( "Expproc", "dwnldComplete  \nCompleted" ); */
	  sprintf(dspdlstat,"dwnldComplete  \nCompleted\n%s\n",vmeAddrAscii);
	  deliverMessage( "Expproc", dspdlstat);
        }
	else
        {
	  /* deliverMessage( "Expproc", "dwnldComplete  \nFailed" ); */
	  sprintf(dspdlstat,"dwnldComplete  \nFailed\n%s\n",vmeAddrAscii);
	  deliverMessage( "Expproc", dspdlstat);
        }
    }
    else
    {
        errLogRet( ErrLogOp, debugInfo, "SendVME failed, console response: %s\n", response );
	/* deliverMessage( "Expproc", "dwnldComplete  \nFailed" ); */
	sprintf(dspdlstat,"dwnldComplete  \nFailed\n%s\n",vmeAddrAscii);
	deliverMessage( "Expproc", dspdlstat);
    }
    mClose( md );
    return( 0 );
}
