/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef INCpatternarg
#define INCpatternarg

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _patarg {
		 	void *callbackRoutine;
		 	void *callbackParams;
        	     } HB_PATTERN_ARG;
	
typedef HB_PATTERN_ARG *HB_PATTERN_ARG_ID;

int cntlrAppHB_PatternSub( HB_PATTERN_ARG *pPatternArg );
int cntlrNodeHB_PatternSub(HB_PATTERN_ARG *pPatternArg);

#ifdef __cplusplus
}
#endif
 
#endif
