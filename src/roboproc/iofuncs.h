/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef __IOFUNCS_H__
#define __IOFUNCS_H__

typedef struct _ioDev {
    char *devName;
    int ((*open) (char *devName));
    int ((*read) (int fd, void *msg, int len));
    int ((*write) (int fd, void *msg, int len));
    int ((*close) (int fd));
    int ((*ioctl) (int fd, int action, ... ));
} ioDev;

/* Pointer to Table Entry and File Descriptor of the Active SMS Device */
extern int smsDev;
extern ioDev *smsDevEntry;

/* List of all the devices we know how to talk to */
extern ioDev devTable[];

extern ioDev *setupSmsComms(char *smsPortPath);

extern int open_lan(char *devName);
extern int open_as_lan(char *devName);
extern int read_lan(int fd, void *msg, int len);
extern int read_lan_timed(int sock, void *buf, int len, int msTimeout);
extern int write_lan(int fd, void *msg, int len);
extern int close_lan(int fd);
extern int ioctl_lan(int fd, int action, ...);

extern int open_serial(char *devName);
extern int read_serial(int fd, void *msg, int len);
extern int write_serial(int fd, void *msg, int len);
extern int close_serial(int fd);
extern int ioctl_serial(int fd, int action, ...);


/* IOCTL Codes */
#define IO_FLUSH    1
#define IO_DRAIN    2

#endif
