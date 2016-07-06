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


typedef struct {
   char	   sibStr[64];
   int	   sibPort;
   int	   gradId;
   int	   sibID;
   int     sibVersion;
   int     sibStatus;
   int     sibBypass;
   int     sibRfMonitor[8];
   SEM_ID  sibMutex;
} SIB_OBJ;

typedef struct {		// Error Code Structure
   unsigned int	low_bits;	// If these bits are low ...
   unsigned int bypass_bits;	// and these are low in bypass ...
   unsigned int	config;		// and these bit are hi in config ...
   unsigned int error_code;	// then send this error code
} SibError;

typedef struct {		// tie all together
   SibError	*errors;
   int		count;
} ErrorRegs;


#if defined(__STDC__) || defined(__cplusplus)
    /* --------- ANSI/C++ function prototypes --------------- */
    void      startSibTask(void);
    void      sibShow(void);
    SIB_OBJ * sibCreate(void);
    void      sibDelete(void);
    int       sibGetId(void);
    int       sibGet(int);
    int       sibPut(int,int);
    static int       sibCollectG(int, int *);
    void       sibShowG(int);
    void       sibTask(int);
    void       sibReset();
#else
    /* --------- K&R prototypes ------------  */
    void      startSibTask();
    void      sibShow();
    SIB_OBJ * sibCreate();
    void      sibDelete();
    int       sibGetId();
    int       sibGet();
    int       sibPut();
    static int       sibCollectG();
    void      sibShowG();
    void      sibTask();
    void      sibReset();
#endif /* __STDC__ */

