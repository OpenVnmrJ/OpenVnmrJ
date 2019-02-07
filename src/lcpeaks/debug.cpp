/* 
 * Varian,Inc. All Rights Reserved.
 * This software contains proprietary and confidential
 * information of Varian, Inc. and its contributors.
 * Use, disclosure and reproduction is prohibited without
 * prior consent.
 */
/* DISCLAIMER :
                 * ------------
                 *
 * This is a beta version of the GALAXIE integration library.
 * This code is under development and is provided for information purposes.
 * The classes names and interfaces as well as the file names and
 * organization is subject to changes. Moreover, this code has not been
 * fully tested.
 *
 * For any bug report, comment or suggestion please send an email to
 * gilles.orazi@varianinc.com
 *
 * Copyright Varian JMBS (2002)
 */

// $History: debug.cpp $
/*  */
/* *****************  Version 5  ***************** */
/* User: Go           Date: 22/10/02   Time: 11:49 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DIAMIR - 6566 : integration : switch to the C++ dll */
/*  */
/* *****************  Version 4  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 3  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:53 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 2  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */

using namespace std;
#include "common.h"
#include <iostream>
#include <fstream>
#include "debug.h"

//Looking for memory leaks
#ifdef __VISUAL_CPP__
#include "LeakWatcher.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif


int _DEBUG_PRIORITY = 1 ;

std::ostream* cdebug = NULL ;
std::ofstream* pDbgfile ;

/* --------------------------
         * dbgstream
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose : Returns the current debug stream
 * History :
 * -------------------------- */

std::ostream& dbgstream()
{
    if (cdebug==NULL)
        return std::cout;
    else
        return *cdebug ;
}

/* --------------------------
         * DbgOutputInFile
 * Author  : Gilles Orazi
 * Created : 10/2002
 * Purpose : Set the current debug stream
 *           into a text file
 * History :
 * -------------------------- */
void DbgOutputInFile(char* filename)
{
    pDbgfile = new std::ofstream(filename);
    setDbgStream(pDbgfile);
}

/* --------------------------
         * DbgCloseFile
 * Author  : Gilles Orazi
 * Created : 10/2002
 * Purpose : Close the debug file
 * History :
 * -------------------------- */
void DbgCloseFile()
{
    setDbgStream(NULL);
    pDbgfile->close() ;
    delete pDbgfile ;
}

/* --------------------------
         * setDbgStream
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose : Set the current debug stream
 * History :
 * -------------------------- */

void setDbgStream(std::ostream* astream)
{
    cdebug = astream ;
}

/* --------------------------
                     * OptionalDebug
     * Author  : Gilles Orazi
     * Created : 06/2002
     * Purpose : Write to debug stream
     *           if priority is low enought
     * History :
     * -------------------------- */
void OptionalDebug(char* message, int priority )
{
    extern int _DEBUG_PRIORITY ;
    if (priority <= _DEBUG_PRIORITY)
        dbgstream() << ".   dbg : "
    << message
    << std::endl; ;
}

/* --------------------------
                     * SetVerboseLevel
     * Author  : Gilles Orazi
     * Created : 06/2002
     * Purpose : Change the max priority for OptionalDebug
     * History :
     * -------------------------- */
void SetVerboseLevel(int priority)
{
    extern int _DEBUG_PRIORITY ;
    _DEBUG_PRIORITY = priority ;
}

