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

#ifndef upLink_h
#define upLink_h

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
                long srcIndex;
                unsigned long dataAddr;
                unsigned long np;
                unsigned long tag;
		unsigned long crc32chksum;
               }  DSP_MSG;

typedef struct {
                long tag;
                long donecode;
                long errorcode;
                long dspIndex;
                unsigned long crc32chksum;
               }  PUBLSH_MSG;

/* --------- ANSI/C++ compliant function prototypes --------------- */

int startDataPublisher(int priority, int taskoptions, int stacksize);

#ifdef __cplusplus
}
#endif

#endif
