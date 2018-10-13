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

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include "CGLProgram.h"

static  bool  isExtensionSupported ( const char * ext, const char * extList )
{
    const char * start = extList;
    const char * ptr;
    while ( ( ptr = strstr ( start, ext ) ) != NULL )
    {    // we've found, ensure name is exactly ext
        const char * end = ptr + strlen ( ext );
        if ( isspace ( *end ) || *end == '\0' )
            return true;
        start = end;
    }
    return false;
}

bool isExtensionSupported (const char * ext )
{
    const char * extensions = (const char *) glGetString (GL_EXTENSIONS);

    if (isExtensionSupported ( ext, extensions ) )
	    return true;
	return false;
}

CGLProgram *getShader(int type){
    switch(type){
    default:
    case NONE:
        return NULL;
    case ARB:
        return new ARBProgram();
    case GLSL:
        return new GLSLProgram();
    }
}


