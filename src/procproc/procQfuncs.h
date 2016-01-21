/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCprocqfuncsh
#define INCprocqfuncsh

#define QENTRIES 1000

/* Processing Queues size & priorities */
#define WEXP_WAIT 0
#define WEXP_WAIT_Q_SIZE  20
#define WEXP_WAIT_PRI  	1

#define WEXP 1
#define WEXP_Q_SIZE  50
#define WEXP_PRI  2

#define WERR 2
#define WERR_Q_SIZE 20
#define WERR_PRI  0

#define WFID 3
#define WFID_Q_SIZE 3
#define WFID_PRI 3

#define WBS 4
#define WBS_Q_SIZE  3
#define WBS_PRI  4

/* Werr, Wexp(wait), Wexp, Wnt, Wbs */
#define NUM_OF_QUEUES 5

#define NUM_IN_ACT_Q	5

#define BG 1
#define FG 2

#define FGREPLY 5

#define OK	0
#define SKIPPED -1

#define EXPID_LEN  256

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* HIDDEN */

/* typedefs */

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

 
extern	int  initProcQs(int clean);
extern	int  procQadd(int proctype, char* expidstr, long elemId, long ct,
                      int dcode, int ecode);
extern  int  procQget(int* proctype, char* expidstr, long* elemId, long* ct,
                      int* dcode, int* ecode);
extern  int  procQentries(void);
extern	int  procQdelete(int proctype, int entry);
extern	int  procQclean(void);
extern  void procQRelease(void);
extern	void procQshow(void);
extern  int  initActiveQ(int clean);
extern  int  activeQadd(char* expidstr, int proctype, long elemId, long ct, int FgBg,
                        int procpid, int dcode, int ecode);
extern  int  activeQget(int* proctype, char* expidstr, int* fgbg, int* pid, int* dcode);
extern  int  activeProcQentries(void);
extern  int  activeQdelete(int fgbg, long key);
extern  int  activeQclean(void);
extern  void activeQRelease(void);
extern  void  activeQshow(void);
 
/* --------- NON-ANSI/C++ prototypes ------------  */

#else
 
extern	int  initProcQs();
extern	int  procQadd();
extern	int  procQget();
extern  int  procQentries();
extern	int  procQdelete();
extern	int  procQclean();
extern  void procQRelease();
extern	void procQshow();
extern  int  initActiveQ();
extern  int  activeQadd();
extern  int  activeQget();
extern  int  activeProcQentries(void);
extern  int  activeQdelete();
extern  int  activeQclean();
extern  void activeQRelease();
extern  void  activeQshow();
 
#endif  /* __STDC__ */
 
#ifdef __cplusplus
}
#endif

#endif /* INCprocqfuncsh */
