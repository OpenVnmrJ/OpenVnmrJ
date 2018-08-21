/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <string.h>
#include <stdlib.h>

#include "aipStderr.h"
#include "aipCommands.h"
using namespace aip;

namespace {
    int debug = 0;
}

int
aipSetDebug(int argc, char *argv[], int retc, char *retv[])
{
    bool set = true;

    if (argc < 2) {
        fprintf(stderr, "%s([['on',] bit# [, bit#...]]", argv[0]);
        fprintf(stderr, " ['off', bit# [, bit#...]] ['none'])\n");
        return proc_complete;
    }
        
    for (argc--, argv++; argc; argc--, argv++) {
	if (strcmp(*argv, "off") == 0) {
	    set = false;
	} else if (strcmp(*argv, "on") == 0) {
	    set = true;
	} else if (strcmp(*argv, "none") == 0) {
	    debug = 0;
            set = true;
	} else {
	    int b = atoi(*argv);
	    if (0 <= b && b <= 31) {
		if (set) {
		    debug |= (1 << b);
		} else {
		    debug &= ~(1 << b);
		}
	    }
	}
    }
    fprintf(stderr,"debug=0x%x\n", debug);/*CMP*/
    return proc_complete;
}
    
int
aipGetDebug()
{
    return debug;
}

void
aipDprint(int bit, const char *format)
{
    if (debug && debug & (1 << bit)) {
	fprintf(stderr,"%s", format);
    }
}

bool
isDebugBit(int bit)
{
    if (debug && debug & (1 << bit)) {
	return true;
    }
    return false;
}
