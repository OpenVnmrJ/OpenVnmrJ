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

#ifndef threadfuncs_h
#define threadfuncs_h

#ifndef RTI_NDDS_4x

#ifndef ndds_cdr_h
#include "ndds/ndds_cdr.h"
#endif

#endif /* RTI_NDDS_4x */

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

#include "nddsbufmngr.h"


#define MAX_CREW_SIZE 48
#define CREW_CNTRLID_SIZE 32
#define CREW_LABEL_SIZE 32
#define CREW_FILEPATH_SIZE 256

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
    int			numSubcriber4Pub;   /* number of subscriber for publication should be just one */
    NDDS_ID		SubId;
    NDDSBUFMNGR_ID      pNddsBufMngr;
    int			SubPipeFd[2];
    NDDS_ID		SubHBId;	    /* Heart Beat Sub from node the thread is servicing */
    int                 cmd;
    int                 number;
    int                 size;
    void               *pParam;
    char                label[32];
    char		filepath[256];  /* directory path to tables and codes */
    struct cntlr_crew  *crew;          /* Pointer to crew struct */
} cntlr_t, *pCntlr_t;

/*
 * The external "handle" for a work crew. Contains the
 * crew synchronization state and staging area.
 */
typedef struct cntlr_crew {
    NDDS_ID		domainID;
    int                 crew_size;      /* Size of array */
    cntlr_t             crew[MAX_CREW_SIZE];/* Crew members */
    long                work_count;     /* Count of work items */
    int			cmd[MAX_CREW_SIZE];	/* cmd to each thread threads */
    pthread_mutex_t     mutex;          /* Mutex for crew data */
    pthread_cond_t      done;           /* Wait for crew done */
    pthread_cond_t      cmdgo;             /* Wait for work */
} cntlr_crew_t, *cntlr_crew_p;


extern int findCntlr(cntlr_crew_t *pCrew, char *cntlrName);
extern int addCntrlThread(cntlr_crew_t *pCrew, char *cntlrName);
extern int initCrew(cntlr_crew_t *pCrew);
#ifndef RTI_NDDS_4x
extern int addCntrlThread(cntlr_crew_t *pCrew, char *cntlrName);
#else  /* RTI_NDDS_4x */
extern int initCntrlThread(cntlr_crew_t *pCrew, char *cntlrName);
extern int createCntrlThread(cntlr_crew_t *pCrew, int theadIndex, NDDSBUFMNGR_ID pNddsBufMngr);
#endif /* RTI_NDDS_4x */

#ifdef __cplusplus
}
#endif

#endif

