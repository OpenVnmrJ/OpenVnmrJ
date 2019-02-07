/* 
 * Varian,Inc. All Rights Reserved.
 * This software contains proprietary and confidential
 * information of Varian, Inc. and its contributors.
 * Use, disclosure and reproduction is prohibited without
 * prior consent.
 */

#include <windows.h>
#include <crtdbg.h>

bool APIENTRY DllMain(HANDLE hModule, 
                      DWORD  ul_reason_for_call, 
                      LPVOID lpReserved)
{
    if( ul_reason_for_call == DLL_PROCESS_DETACH)
        _CrtDumpMemoryLeaks();
    return true;
}


