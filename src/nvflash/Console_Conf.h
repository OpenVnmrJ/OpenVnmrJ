
/*
  WARNING: THIS FILE IS AUTO-GENERATED. DO NOT MODIFY.

  This file was generated from Console_Conf.idl using "rtiddsgen".
  The rtiddsgen tool is part of the RTI Data Distribution Service distribution.
  For more information, type 'rtiddsgen -help' at a command shell
  or consult the RTI Data Distribution Service manual.
*/

#ifndef Console_Conf_h
#define Console_Conf_h

#ifndef NDDS_STANDALONE_TYPE
    #ifdef __cplusplus
        #ifndef ndds_cpp_h
            #include "ndds/ndds_cpp.h"
        #endif
    #else
        #ifndef ndds_c_h
            #include "ndds/ndds_c.h"
        #endif
    #endif
#else
    #include "ndds_standalone_type.h"
#endif


/*

*

* Varian,Inc. All Rights Reserved.

* This software contains proprietary and confidential

* information of Varian, Inc. and its contributors.

* Use, disclosure and reproduction is prohibited without

* prior consent.

*/

/*

*  Author: Greg Brissey  11/05/2007

*/

#include "NDDS_Obj.h"
                
#define CONSOLE_CONF_STRUCT_VERSION (1)                
                        
#define CNTLR_PUB_CONF_TOPIC_FORMAT_STR ("master/h/conconf")                
                        
#define HOST_SUB_CONF_TOPIC_FORMAT_STR ("master/h/conconf")                
                        
#define CONSOLE_CONF_MAX_STR_LEN (64)                
        
#ifdef __cplusplus
extern "C" {
#endif

        
extern const char *Console_ConfTYPENAME;
        

#ifdef __cplusplus
}
#endif

typedef struct Console_Conf
{
    DDS_Long  structVersion;
    DDS_Long  ConsoleTypeFlag;
    DDS_Long  SystemRevId;
    char*  VxWorksVersion; /* maximum length = ((CONSOLE_CONF_MAX_STR_LEN)) */
    char*  RtiNddsVersion; /* maximum length = ((CONSOLE_CONF_MAX_STR_LEN)) */
    char*  PsgInterpVersion; /* maximum length = ((CONSOLE_CONF_MAX_STR_LEN)) */
    char*  CompileDate; /* maximum length = ((CONSOLE_CONF_MAX_STR_LEN)) */
    char*  ddrmd5; /* maximum length = ((CONSOLE_CONF_MAX_STR_LEN)) */
    char*  gradientmd5; /* maximum length = ((CONSOLE_CONF_MAX_STR_LEN)) */
    char*  lockmd5; /* maximum length = ((CONSOLE_CONF_MAX_STR_LEN)) */
    char*  mastermd5; /* maximum length = ((CONSOLE_CONF_MAX_STR_LEN)) */
    char*  nvlibmd5; /* maximum length = ((CONSOLE_CONF_MAX_STR_LEN)) */
    char*  nvScriptmd5; /* maximum length = ((CONSOLE_CONF_MAX_STR_LEN)) */
    char*  pfgmd5; /* maximum length = ((CONSOLE_CONF_MAX_STR_LEN)) */
    char*  lpfgmd5; /* maximum length = ((CONSOLE_CONF_MAX_STR_LEN)) */
    char*  rfmd5; /* maximum length = ((CONSOLE_CONF_MAX_STR_LEN)) */
    char*  vxWorksKernelmd5; /* maximum length = ((CONSOLE_CONF_MAX_STR_LEN)) */
    char*  fpgaLoadStr; /* maximum length = ((CONSOLE_CONF_MAX_STR_LEN)) */

} Console_Conf;
    
                            
#if (defined(RTI_WIN32) || defined (RTI_WINCE)) && defined(NDDS_USER_DLL_EXPORT)
  /* If the code is building on Windows, start exporting symbols.
   */
  #undef NDDSUSERDllExport
  #define NDDSUSERDllExport __declspec(dllexport)
#endif

    

DDS_SEQUENCE(Console_ConfSeq, Console_Conf);
        
NDDSUSERDllExport
RTIBool Console_Conf_initialize(
        Console_Conf* self);
        
NDDSUSERDllExport
RTIBool Console_Conf_initialize_ex(
        Console_Conf* self,RTIBool allocatePointers);

NDDSUSERDllExport
void Console_Conf_finalize(
        Console_Conf* self);
                        
NDDSUSERDllExport
void Console_Conf_finalize_ex(
        Console_Conf* self,RTIBool deletePointers);
        
NDDSUSERDllExport
RTIBool Console_Conf_copy(
        Console_Conf* dst,
        const Console_Conf* src);

#if (defined(RTI_WIN32) || defined (RTI_WINCE)) && defined(NDDS_USER_DLL_EXPORT)
  /* If the code is building on Windows, stop exporting symbols.
   */
  #undef NDDSUSERDllExport
  #define NDDSUSERDllExport
#endif


#ifdef __cplusplus

    extern "C" {

#endif



extern void getConsole_ConfInfo(NDDS_OBJ *pObj);



#ifdef __cplusplus

}

#endif


#endif /* Console_Conf_h */
