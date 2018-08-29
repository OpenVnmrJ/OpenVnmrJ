/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* Test, debug, evaluation and demonstration program -  not part of any product. */

/* Everything in this file is subject to change without any notice. */

#include "chanLib.h"

#define  NOCMD		0
#define  QUIT		1
#define  READDATA	2
#define  WRITEDATA	3
#define  READWRITE	4
#define  WRITEREAD	5
#define  PAUSENREAD	6
#define  PAUSENWRITE	7
#define  PAUSENRW	8
#define  PAUSENWR	9
#define  FIRST_COMMAND  QUIT
#define  LAST_COMMAND   PAUSENWR


#define  DEFAULT_BLOCK_SIZE	512

#define  INHERIT_COMMAND_DATA			\
	int	cmd;				\
	int	size;				\
	int	count;
		
struct  command_data {
	INHERIT_COMMAND_DATA
};

typedef union {
	struct command_data	cmd_data;
	char			cmd_space[ DEFAULT_BLOCK_SIZE ];
} command_block;


#define  PROCEED	0
#define  STOP		1

#define  NO_MEMORY	1

typedef struct {
	int	reply;
	int	reason;
} reply_stuff;

typedef union {
	reply_stuff	reply_data;
	char		reply_space[ DEFAULT_BLOCK_SIZE ];
} reply_block;
