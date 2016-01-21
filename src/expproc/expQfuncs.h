/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCexpqfuncsh
#define INCexpqfuncsh

/* Processing Queues size & priorities */

#define OK 0

#define HIGHPRIO 0
#define NORMALPRIO 1

#define NUM_EXP_OF_QUEUES 2
#define EXP_Q_SIZE 500

#define NUM_IN_ACT_Q	5

#define EXPID_LEN  256

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* HIDDEN */

/* typedefs */

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

 
extern int initExpQs(int clean);
extern int expQaddToTail(int priority, char* expidstr,
                         char* expinfostr);
extern int expQget(int* priority, char* expidstr);
extern int expQgetinfo(int index, char* expinfostr);
extern int expQsearch(char *userName,char* expnum,int *priority,char *expidstr);
extern int expQIdsearch(char *expidstr, int *priority);
extern int expQentries();
extern int expQdelete(int priority, char *idstr);
extern void expQclean(void);
extern  void ExpQRelease(void);
extern	int  ExpQshow(void);
extern  int  initActiveExpQ(int clean);
extern  int  activeExpQadd(int priority, char* expidstr,
                           char* expinfostr);
extern  int  activeExpQget(int* priority, char* expidstr);
extern  int  activeExpQentries();
extern  int  activeExpQdelete(char *Expidstr);
extern  int  activeExpQclean(void);
extern  void activeExpQRelease(void);
extern  int  activeExpQshow(void);
 
/* --------- NON-ANSI/C++ prototypes ------------  */

#else
 
extern	int  initExpQs();
extern	int  expQaddToTail();
extern	int  expQget();
extern  int expQgetinfo();
extern int expQsearch();
extern  int expQentries();
extern	int  expQdelete();
extern	int  expQclean();
extern  void expQRelease();
extern	int  expQshow();
extern  int  initActiveExpQ();
extern  int  activeExpQadd();
extern  int  activeExpQget();
extern  int  activeExpQentries();
extern  int  activeExpQdelete();
extern  int  activeExpQclean();
extern  void activeExpQRelease();
extern  int  activeExpQshow();
 
#endif  /* __STDC__ */
 
#ifdef __cplusplus
}
#endif

#endif /* INCexpqfuncsh */
