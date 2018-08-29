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

/*
 *  Constants defining the rescheduling of acqproc polling
 */

/*
 *   CONTANT        FUNCTION
 *   =============  ========
 *   TIME_OFF       turn off timer
 *   TIME_ON        initalize service routine for time interupts and
 *                  then do a TIME_RESET
 *   TIME_RESET     set timing interval to 5 seconds and start timer
 *   TIME_NORMAL    use current timing interval and start timer
 *   TIME_FAST      set timing interval to 0.01 seconds and start timer
 *   TIME_FASTER    decrease timing interval by 0.05 seconds and start timer
 *   TIME_SLOWER    increase timing interval by 0.01 seconds if an experiment
 *                  is active and by 0.05 seconds if no experiment is active
 *                  and start timer
 *   TIME_NEXT_FAST set the next current timing interval to 0.01 seconds but
 *                  do NOT set timer
 *   TIME_NEXT_SLOW set the next current timing interval to 5 seconds but
 *                  do NOT set timer
 *
 */

#define TIME_OFF       1
#define TIME_ON        2
#define TIME_RESET     3
#define TIME_NORMAL    4
#define TIME_FAST      5
#define TIME_FASTER    6
#define TIME_SLOWER    7
#define TIME_NEXT_FAST 8
#define TIME_NEXT_SLOW 9
