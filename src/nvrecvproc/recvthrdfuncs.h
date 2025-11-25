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
#ifndef recvthrdfuncs_h
#define recvthrdfuncs_h

#ifndef RTI_NDDS_4x
  #ifndef ndds_cdr_h
  #include "ndds/ndds_cdr.h"
  #endif
#endif  /* RTI_NDDS_4x */

#ifndef ndds_c_h
#include "ndds/ndds_c.h"
#endif

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef NDDS_Obj_h
#include "NDDS_Obj.h"
#endif

#include "mfileObj.h"
#include "rngBlkLib.h"
#include "rcvrDesc.h"
#include "workQObj.h"

#define MAX_CREW_SIZE 16
#define MAX_PIPE_STAGES 5
/* #define MAX_WORKQ_ENTRIES 20 */
#define MAX_WORKQ_ENTRIES 100
/* #define QUEUE_LENGTH 20 */
#define QUEUE_LENGTH 100
#define CREW_CNTRLID_SIZE 32
#define CREW_LABEL_SIZE 32
#define CREW_FILEPATH_SIZE 256

/* typedef int (*PFIV)(void*, void *arg); */

/* this is the struct handed to each thread stage that is in the pipe line */
typedef struct pipe_stage {
    pthread_t               threadId;
    char                   *cntlrId;   /* controller Id, master,rf1, etc.. */
    int                     active;		/* active data elements */
    int                     stage;		/* which stage this thread is */
    int                     stages;		/* number of stages in pipe line */
    PFIV                    pCallbackFunc;   /* routine that pipe stage thread calls to do actually work of this stage */
    WORKQ_ID                pWorkQObj;	/* pointer to work area */
    WORKQ_ENTRY_ID          activeWrkQEntry;  /* present work q entry being built by the NDDS call back */
    RINGBLK_ID              pInputQ;	       /* Input Q for this pipe stage */
    RINGBLK_ID              pOutputQ;	/* Output Q for this pipe stage */
    struct cntlr_thread    *pThisCrew;	/* reference back to crew, for unforseen needs */
    void                   *pParam;
    void                   *pFidBlockHeader;
    unsigned int           prevElemId;   /* used to checking lost issues */
 }  PIPE_STAGE_OBJECT, *PIPE_STAGE_ID;

/*
 * One of these is initialized for each worker thread in the
 * crew. It contains the "identity" of each worker.
 */
typedef struct cntlr_thread {
    pthread_t		threadId;
    int			index;
    char 		cntlrId[32];   /* controller Id, master,rf1, etc.. */
    NDDS_ID		domainID;
    NDDS_ID		PubId;
    int			numPub4Sub;   /* number of Publishers for subscription, should be just one */
    NDDS_ID		SubId;
    NDDS_ID		SubHBId;     /* Heart Beat Sub from node the thread is servicing */
    int                 cmd;
    int                 number;
    int                 size;
    WORKQ_ID		pWorkQObj;
    PIPE_STAGE_ID       pPipeStages[MAX_PIPE_STAGES];
    int			numStages;
    void               *pParam;
    char                label[32];
    char		filepath[256];  /* directory path to tables and codes */
    struct cntlr_crew  *crew;          /* Pointer to crew struct */
} cntlr_t, *pCntlr_t;

/* notes:
 * The external "handle" for a work crew. Contains the
 * crew synchronization state and staging area.
 */
typedef struct cntlr_crew {
    NDDS_ID		domainID;
    int                 crew_size;      /* Size of array */
    cntlr_t             crew[MAX_CREW_SIZE];/* Crew members */
    int                work_count;     /* Count of work items */
    int			cmd[MAX_CREW_SIZE];	/* cmd to each thread threads */
    pthread_mutex_t     mutex;          /* Mutex for crew data */
    pthread_cond_t      done;           /* Wait for crew done */
    pthread_cond_t      cmdgo;             /* Wait for work */
    int		        wrkdone;        /* predicate param for conditional done */
} cntlr_crew_t, *cntlr_crew_p;

typedef struct cntlr_status {
    int  numCntlrs;			/* number of cntlr threads that maybe active */
    int  cntlrsActive;			/* count of active threads */
    int  waiting4Done;			/* thread waiting for done */
    pthread_mutex_t     mutex;          /* Mutex for crew data */
    pthread_cond_t      done;           /* Wait for crew done */
    int		        wrkdone;        /* predicate param for conditional done */
} cntlr_status_t, *cntlr_status_p;
   

/* extern int findCntlr(cntlr_crew_t *pCrew, char *cntlrName); */
extern int findCntlr(char *cntlrName);
/* extern int addCntrlThread(cntlr_crew_t *pCrew, char *cntlrName); */
extern int addCntrlThread(RCVR_DESC_ID pWrkDesc, char *cntlrName);
extern int initCrew(cntlr_crew_t *pCrew);
extern int initCntlrStatus();
extern int resetCntlrStatus();
extern int incrActiveCntlrStatus(char *cntlrId);
extern int decActiveCntlrStatus(char *cntlrId);
extern int wait4DoneCntlrStatus();

#ifdef __cplusplus
}
#endif

#endif

