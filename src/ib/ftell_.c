/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include "f2c.h"
#include "fio.h"

 static FILE *
#ifdef KR_headers
unit_chk(Unit, who) integer Unit; char *who;
#else
unit_chk(integer Unit, char *who)
#endif
{
	if (Unit >= MXUNIT || Unit < 0)
		f__fatal(101, who);
	return f__units[Unit].ufd;
	}

 integer
#ifdef KR_headers
ftell_(Unit) integer *Unit;
#else
ftell_(integer *Unit)
#endif
{
	FILE *f;
	return (f = unit_chk(*Unit, "ftell")) ? ftell(f) : -1L;
	}

 int
#ifdef KR_headers
fseek_(Unit, offset, whence) integer *Unit, *offset, *whence;
#else
fseek_(integer *Unit, integer *offset, integer *whence)
#endif
{
	FILE *f;
	return	!(f = unit_chk(*Unit, "fseek"))
		|| fseek(f, *offset, (int)*whence) ? 1 : 0;
	}
