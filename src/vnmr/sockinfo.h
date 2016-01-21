/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef SOCKINFO_H
#define SOCKINFO_H

extern int getInfoTempExpControl();
extern int getExpStatusInt(int index, int *val);
extern int getExpStatusShim(int index, int *val);
extern int open_ia_stat(char *parpath, int removefile, int valid_test);

extern int setAutoDir(char *str);
extern int getAutoDir(char *str, int maxlen);
extern int setInfoSpinOnOff(int val);
extern int getInfoSpinOnOff();
extern int setInfoSpinSetSpeed(int val);
extern int getInfoSpinSetSpeed();
extern int setInfoSpinUseRate(int val);
extern int getInfoSpinUseRate();
extern int setInfoSpinSetRate(int val);
extern int getInfoSpinSetRate();
extern int setInfoSpinSelect(int val);
extern int getInfoSpinSelect();
extern int setInfoSpinSwitchSpeed(int val);
extern int getInfoSpinSwitchSpeed();

extern int getInfoSpinSpeed();
extern int getInfoSpinner();
extern int setInfoSpinExpControl(int val);
extern int getInfoSpinExpControl();
extern int setInfoSpinErrorControl(int val);
extern int getInfoSpinErrorControl();
extern int setInfoInsertEjectExpControl(int val);
extern int getInfoInsertEjectExpControl();

extern int setInfoTempOnOff(int val);
extern int getInfoTempOnOff();
extern int setInfoTempSetPoint(int val);
extern int getInfoTempSetPoint();
extern int setInfoTempExpControl(int val);
extern int getInfoTempExpControl();
extern int setInfoTempErrorControl(int val);
extern int getInfoTempErrorControl();

extern int send2Acq(int cmd, char *msg_for_acq );

#endif
