/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*---------------------------------------------------
|  acqinfo.h  - generated via rpcgen acqinfo.x
|
|   9-12-89  Changed for SUN OS 403  Greg Brissey
+----------------------------------------------------*/
typedef char *a_string;
bool_t xdr_a_string();


struct acqdata {
	int pid;
	int pid_active;
	int rdport;
	int wtport;
	int msgport;
	char *host;
};
typedef struct acqdata acqdata;
bool_t xdr_acqdata();

struct ft3ddata {
        char *autofilepath;
	char *procmode;
        char *username;
	char *pathenv;
};
typedef struct ft3ddata ft3ddata;
bool_t xdr_ft3ddata();


#define ACQINFOPROG ((u_long)99)
#define ACQINFOVERS ((u_long)2)
#define ACQINFO_GET ((u_long)1)
extern acqdata *acqinfo_get_2();
#define ACQPID_PING ((u_long)2)
extern int *acqpid_ping_2();
#define FT3D_START ((u_long)3)
extern int *ft3d_start_2();
