/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCmboxcmdh
#define INCmboxcmdh

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/*
DESCRIPTION

Automation 332 Mail Box Commands 

*/
#define MBOX_CMD_SIZE short

#define GET_LOCK_VALUE 1
#define LOCK_VALUE_SIZE short

#define GET_SHIMS_PRESENT 2
#define GET_SERIAL_SHIMS_PRESENT 3
#define SHIMS_PRESENT_SIZE short

#define GET_SPIN_VALUE 10
#define SPIN_VALUE_SIZE int

#define EJECT_SAMPLE 20
#define INSERT_SAMPLE 21

#define SET_SPIN_RATE 22
#define SET_SPIN_SPEED 25
#define SET_SPIN_MAS_THRESHOLD 27
#define SET_SPIN_REG_DELTA 28
#define SET_SPIN_ARG_SIZE int

#define BEARING_ON    23
#define BEARING_OFF   24
#define SAMPLE_DETECT 26

#define SET_DEBUGLEVEL 30
#define DEBUGLEVEL_VALUE_SIZE short

/* Keep values in the following block sequential */
#define SET_GPA_TUNE_PX 31
#define SET_GPA_TUNE_IX 32
#define SET_GPA_RESERVED_X 33
#define SET_GPA_TUNE_PY 34
#define SET_GPA_TUNE_IY 35
#define SET_GPA_RESERVED_Y 36
#define SET_GPA_TUNE_PZ 37
#define SET_GPA_TUNE_IZ 38
#define SET_GPA_RESERVED_Z 39
#define SET_GPAENABLE 40
/* End sequential value block */
#define GPA_TUNE_VALUE_SIZE short

#define GPA_DISABLE_DELAY (-300) /* In -seconds */
#define GPA_ENABLE_OFF 0
#define GPA_ENABLE_ON 1

#define THIN_SHIMS_ON 41
#define THIN_SHIMS_OFF 42
#define EJECT_SAMPLE_OFF 43

/* Error Codes */
#define MUTEX_TIME_OUT  -10
#define CMD_TIME_OUT    -20

#ifdef __cplusplus
}
#endif

#endif
