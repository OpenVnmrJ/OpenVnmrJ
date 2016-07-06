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

#ifndef INCxinterph
#define INCxinterph


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif



/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

void Xparser(void);
void X_interp(char *);
void shimHandler     (int *, int *, int, int);
void spinnerHandler  (int *, int *, int);
void set2sw_fifo(int);
int  auxReadReg(int);
void auxWriteReg(int,int);

int chlock();				// Is the system locked
int get_lock_offset();			// return z0 for now
int get_limit_lock();	
int getgain();
int getpower();
int getphase();
int getmode();
double get_lkfreq_ap();
void set_lock_offset( int newz0 );
void set_lk_freq_ap( double newfreq );
void setgain(int newgain);
void setpower(int newpower);
void setphase(int newphase);
void setmode(int newmode);
void lk2kcf();
void lk2kcs();
void lk20Hz();

void locktc();
void lockacqtc();

extern void spiShimInterface(int *, int *, int, int);
extern void qspiShimInterface(int *, int *, int, int);
extern int establishShimType(int);
extern syncHandler();
extern statHandler();
extern statTimer();
extern nvRamHandler();
extern tuneHandler();
extern autshmHandler();
extern rcvrgainHandler();
extern fixAcodeHandler();
extern setAttnHandler();
extern setvtHandler();
extern shmsetHandler();
extern lkfreqHandler();

#else                                                   
/* --------- NON-ANSI/C++ prototypes ------------  */

void Xparser();
void X_interp();
static int doOneXcommand();
void shimHandler();
void spinnerHandler();
void set2sw_fifo();
int  auxReadReg();
void auxWriteReg();

extern void spiShimInterface();
extern void qspiShimInterface();
extern int establishShimType();
extern syncHandler();
extern statHandler();
extern statTimer();
extern nvRamHandler();
extern tuneHandler();
extern autshmHandler();
extern rcvrgainHandler();
extern fixAcodeHandler();
extern setAttnHandler();
extern setvtHandler();
extern shmsetHandler();
extern lkfreqHandler();

#endif

#define CONSOLE_MSGE_SIZE	(512)

#ifdef __cplusplus
}
#endif

#endif

