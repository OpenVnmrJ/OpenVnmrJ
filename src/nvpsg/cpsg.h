/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* the extern "C" declaration makes these avalaibale to C++ functions */


extern "C" {

#include "group.h"
#include "pvars.h"
#include "cps.h"

#define TRUE  1
#define FALSE 0
#define MAXSTR 256

void setupsignalhandler();
int P_receive(int *);
int A_getnames(int, char **, int *, int);
int A_getvalues(int, double **, int *, int);
short whattype(int, const char *);
double whatamphibandmin(int rfchan, double freq);
double getExtraLoopTime();
void init_hash(int);
void load_hash(char **, int);
int option_check(const char *);
int gradtype_check();
void initparms();
void initacqparms(unsigned int);
void initautostruct(void);
void pre_expsequence(void);
void initlpelements(void);
void printlpel(void);
void initglobalptrs(void);
int parse(char *, int *);
void arrayPS(int,int,int);
void first_done();
void write_exp_info();
void write_shr_info(double);
int closeCmd();
void closepipe2(int);
void reset(void);
int setup2D(double);
int setup4D(double,double,double,char *,char *);
int setPulseFilter();
int QueueExp(char *codefile,int nextflag);
void read_info_file(char *autodir);
void read_sample_info(FILE *stream, struct sample_info *s);
void write_sample_info(FILE *stream, struct sample_info *s);
void get_sample_info(sample_info *s,const char *keyword,char *info,int size,int *entry);
void check_for_abort();
int sendasync(char *addr,char *message);
void setup_comm(void);   /* setup communications with acquisition processes */
int sendExpproc(char *acqaddr,char *filename,char *info,int nextflag);
int sendNvExpproc(char *acqaddr,char *filename,char *info,int nextflag);
int have_imaging_sw(void);
void validate_imaging_config(char *callname);
int is_psg4acqi();
int setup_parfile(int setupflag);
void set_rcvrs_info();
void closeFiles();
void showpowerintegral();

/* Real Time Related */
void G_initval(double, int);
void F_initval(double, int);
int loop(int, int);
void endloop(int);
void loop_check();
void starthardloop(int);
void endhardloop();
void msloop();
void endmsloop();
void peloop();
void peloop2();
void endpeloop();
void endpeloop2();
int  validrtvar(int);
int getcurrentloopcount();
int getloopcount_for_index(int);
int get_acqvar(int);

/* Table related */
void doWriteTable(int); /* aptable.c ensures that phasetable is written to acode */

/* Error Handling */
#include "abort.h"

 }
