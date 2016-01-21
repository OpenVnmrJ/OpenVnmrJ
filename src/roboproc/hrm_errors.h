/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef __HRM_ERRORS_H__
#define __HRM_ERRORS_H__ 1

#define HRM_SUCCESS                    0
#define HRM_OPERATION_ABORTED       -504

#define HRM_NOPOWER                -7000  /* power not enabled */
#define HRM_NOTCALIBRATED          -7001  /* robots not calibrated */
#define HRM_ROBOT1_CANTPARK        -7002  /* robot1 failed move to park pos */
#define HRM_ROBOT2_CANTPARK        -7003  /* robot2 failed move to park pos */
#define HRM_NOGLASS_ROBOT1         -7050  /* vial not present, robot 1 */
#define HRM_NOGLASS_ROBOT2         -7100  /* vial not present, robot 2 */
#define HRM_ROBOT2_NORETRACT       -7101  /* robot2 cylinder not retracted */
#define HRM_ROBOT2_NOEXTEND        -7102  /* robot2 cylinder not extended */
#define HRM_TURBINE_UPB_PRESENT    -7103  /* turbine already in upper barrel*/
#define HRM_MISSTURB_AFTERPLACE    -7104  /* no turbine in upb after drop */
#define HRM_ROBOT2_VIALPRESENT     -7105  /* vial in robot2 gripper already */
#define HRM_ROBOT2_CYLUNKNOWN      -7106  /* robot2 cyl in unknown state */
#define HRM_MISSTURB_BEFOREPICK    -7150  /* no turbine present before pick */
#define HRM_FAILED_TURBINEPICK     -7151  /* turb still present after pick */
#define HRM_TABLE_AJAR             -7200  /* swing-out table not in place */
#define HRM_INVALID_CMD            -7201  /* command is not valid */
#define HRM_INVALID_PARAM          -7202  /* parameter(s) not valid */
#define HRM_DATABASE_ERROR         -7203  /* Database-related Adept error */
#define HRM_IO_ERROR               -7204  /* I/O error */
#define HRM_SERVO_ERROR            -7205  /* Robot servo error */
#define HRM_TIMEOUT_ROBOT1         -7206  /* Timeout waiting for robot1 */
#define HRM_TIMEOUT_ROBOT2         -7207  /* Timeout waiting for robot2 */
#define HRM_GENERIC_ERROR          -7300  /* Generic error code */
#define HRM_POSCALC_ERROR          -7301  /* Err calc'g vial rack positions */
#define HRM_SCHEDULER_ERROR        -7302  /* Err send'g info to robot sched */

#endif
