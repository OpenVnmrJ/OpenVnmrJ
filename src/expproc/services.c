/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>

#include "parser.h"

static
local_listCmds()
{
	int	 iter, clen, mlen;
	cmd	*table;

/*  Measure the length of the longest name in the table  */

	table = addrOfCmdTable();
	mlen = -1;
	for (iter = 0; table[ iter ].n != NULL; iter++) {
		clen = strlen( table[ iter ].n );
		if (clen > mlen)
		  mlen = clen;
	}

	for (iter = 0; table[ iter ].n != NULL; iter++)
	  printf( "%-*s%s\n", mlen+4, table[ iter ].n, table[ iter ].d );
}

main()
{
    local_listCmds( NULL );
}
