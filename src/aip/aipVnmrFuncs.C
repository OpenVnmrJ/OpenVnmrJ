/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <string.h>

#include <algorithm>
using std::min;

#include "variables.h"		// For T_REAL, T_STRING
#include "group.h"		// For GLOBAL, CURRENT, ACT_ON, D_PROCESSING

extern int interuption;        // interuption [sic!] from Vnmr: smagic.c
extern char curexpdir[];        // From vnmrsys.h

#include "aipVnmrFuncs.h"
#include "aipMouse.h"
#include "aipGframeManager.h"
using namespace aip;

namespace {
    bool aipLockedScreen = false;
}

void
grabMouse()
{
    if (!aipHasScreen()) {
        Jturnoff_mouse();
        GframeManager::get()->resizeWindow(false);
    }
    bool lock = aipLockScreen(true);
    Jactivate_mouse(NULL, NULL, NULL, Mouse::event, Mouse::reset);
    if(!is_aip_window_opened())
    Wsetgraphicsdisplay("aipRedisplay");
    Mouse::setState(Mouse::previous);
    aipLockScreen(lock);
    setMenuName("aip");
}

void
releaseMouse()
{
    Jturnoff_mouse();
}

/*
 * Set true to indicate a Wturnoff_mouse call came from us.
 */
bool
aipLockScreen(bool b)
{
    bool oldLock = aipLockedScreen;
    aipLockedScreen = b;
    return oldLock;
}

bool
aipLockScreen()
{
    return aipLockedScreen;
}

bool
aipHasScreen()
{
    char buf[16];
    Wgetgraphicsdisplay(buf, sizeof(buf));
    if (aipLockedScreen || strncmp(buf, "aipRedisplay", sizeof(buf)-1) == 0
	|| is_aip_window_opened()) {
        return true;
    }
    return false;
}

bool
setReal(string varname, int index, double value, bool notify)
{
    const char *cname = varname.c_str();
    if (P_setreal(GLOBAL, cname, value, index)) { // Try GLOBAL
	if (P_setreal(CURRENT, cname, value, index)) { // Try CURRENT
	    if (P_creatvar(GLOBAL, cname, T_REAL)) { // Create in GLOBAL
		return false;	// Can this happen?
	    }
	    P_setprot(GLOBAL, cname, 0x8000); // Set no-share attribute
            P_setdgroup(GLOBAL, cname, D_PROCESSING); // Processing parm
	    if (P_setreal(GLOBAL, cname, value, index)) {
		return false;	// Can this happen?
	    }
	}
    }
    if (notify) {
	appendJvarlist(cname);
    }
    return true;
}

bool
setReal(string varname, double value, bool notify)
{
    const char *cname = varname.c_str();
    if (P_setreal(GLOBAL, cname, value, 1)) { // Try GLOBAL
	if (P_setreal(CURRENT, cname, value, 1)) { // Try CURRENT
	    if (P_creatvar(GLOBAL, cname, T_REAL)) { // Create in GLOBAL
		return false;	// Can this happen?
	    }
	    P_setprot(GLOBAL, cname, 0x8000); // Set no-share attribute
            P_setdgroup(GLOBAL, cname, D_PROCESSING); // Processing parm
	    if (P_setreal(GLOBAL, cname, value, 1)) {
		return false;	// Can this happen?
	    }
	}
    }
    if (notify) {
	appendJvarlist(cname);
    }
    return true;
}

bool
setString(string varname, string value, bool notify)
{
    const char *cname = varname.c_str();
    const char *cval = value.c_str();
    if (P_setstring(GLOBAL, cname, cval, 1)) { // Try GLOBAL
	if (P_setstring(CURRENT, cname, cval, 1)) { // Try CURRENT
	    if (P_creatvar(GLOBAL, cname, T_STRING)) { // Create in GLOBAL
		return false;	// Can this happen?
	    }
	    P_setprot(GLOBAL, cname, 0x8000); // Set no-share attribute
            P_setdgroup(GLOBAL, cname, D_PROCESSING); // Processing parm
	    if (P_setstring(GLOBAL, cname, cval, 1)) {
		return false;	// Can this happen?
	    }
	}
    }
    if (notify) {
	appendJvarlist(cname);
    }
    return true;
}


double
getReal(string varname, double defaultVal)
{
    const char *cname = varname.c_str();
    double value;
    if (P_getreal(GLOBAL, cname, &value, 1)) { // Try GLOBAL
	if (P_getreal(CURRENT, cname, &value, 1)) { // Try CURRENT
	    // Doesnt exist; create it with default value
	    setReal(varname, defaultVal, false);
	    return defaultVal;
	}
    }
    return value;
}

double
getReal(string varname, int index, double defaultVal)
{
    const char *cname = varname.c_str();
    double value;

    if (index <= 0) {
        index = 1;
    }
    if (P_getreal(GLOBAL, cname, &value, index)) { // Try GLOBAL
	if (P_getreal(CURRENT, cname, &value, index)) { // Try CURRENT
	    // Doesnt exist
	    return defaultVal;
	}
    }
    return value;
}

string
getString(string varname)
{
    const char *cname = varname.c_str();
    char cval[2048];
    if (P_getstring(GLOBAL, cname, cval, 1, sizeof(cval))) {
	if (P_getstring(CURRENT, cname, cval, 1, sizeof(cval))) {
            // Doesn't exist; return empty string
            // (Don't create variable)
	    return "";
	}
    }
    string rtn(cval);
    return rtn;
}

string
getString(string varname, string defaultVal)
{
    const char *cname = varname.c_str();
    char cval[2048];
    if (P_getstring(GLOBAL, cname, cval, 1, sizeof(cval))) {
	if (P_getstring(CURRENT, cname, cval, 1, sizeof(cval))) {
	    // Doesnt exist; create it with default value
	    setString(varname, defaultVal, false);
	    return defaultVal;
	}
    }
    string rtn(cval);
    return rtn;
}

bool
isActive(string varname)
{
    const char *cname = varname.c_str();
    int stat;
    if ((stat=P_getactive(GLOBAL, cname)) == ACT_ON ||
	(stat < 0 && P_getactive(CURRENT, cname) == ACT_ON))
    {
	return true;
    }
    return false;
}

bool
setActive(string varname, bool value)
{
    const char *cname = varname.c_str();
    int ival = value ? ACT_ON : ACT_OFF;
    if (P_setactive(GLOBAL, cname, ival)) { // Try GLOBAL
	if (P_setactive(CURRENT, cname, ival)) { // Try CURRENT
	    return false;
	}
    }
    return true;
}

string
getCurexpdir()
{
    return (string)curexpdir;
}

/*
 * These five functions are used by image math and are
 * called by mathproto.c when it is dynamically linked in.
 */
void ib_msgline(const char *str)
{
    // If there's a number in here, display it as an index on the status line
    const char *digits="0123456789";
    int i = strcspn(str, digits);
    int j;
    if ((j = atoi(str+i)) != 0 || str[i] == '0') {
        disp_index(j);
    } else {
        fprintf(stderr, str);
    }
}

void ib_errmsg(const char *str)
{
    Werrprintf("%s",str);
}

int interrupt(void)
{
    return interuption;
}

void interrupt_end(void) {}
void interrupt_begin(void) {}
