/*
 * Varian,Inc. All Rights Reserved.
 * This software contains proprietary and confidential
 * information of Varian, Inc. and its contributors.
 * Use, disclosure and reproduction is prohibited without
 * prior consent.
 */
#ifndef IMWATCHINGYOULEAK
#define IMWATCHINGYOULEAK

/*
    Include file to be used when looking for memory leaks
    to get the correct file and line numbers in the output log.
    Tip taken from : http://www.codeproject.com/debug/ConsoleLeak.asp
 */
#include <crtdbg.h>

#ifdef _DEBUG

#define DEBUG_NEW new(_NORMAL_BLOCK,__FILE__,__LINE__)

#define MALLOC_DBG(x) _malloc_dbg(x, 1, THIS_FILE, __LINE__);
#define malloc(x) MALLOC_DBG(x)

#endif // _DEBUG

#endif // #include guard

