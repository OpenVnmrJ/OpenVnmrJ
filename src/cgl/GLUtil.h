/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* %Z%%M% %I% %G% Copyright (c) 1991-1996 Varian Assoc.,Inc. All Rights Reserved
 */

#ifndef GLUTIL_H_
#define GLUTIL_H_
class CGLProgram;

extern bool isExtensionSupported (const char * ext);
extern CGLProgram *getShader(int);

#endif
