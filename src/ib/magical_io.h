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

#ifndef MAGICAL_IO_H
#define MAGICAL_IO_H

#include <stdio.h>

char *magicBp;
node *magicCodeTree;
int magicColumnNumber;
extern char magicFileName[];
int magicFromFile;
int magicFromString;
int magicLineNumber;
char magicMacroBuffer[1024];
FILE *magicMacroFile;
char *magicMacroBp;
int magicInterrupt;
int magicWorking;
char **magicMacroDirList;

#endif /* MAGICAL_IO_H */
