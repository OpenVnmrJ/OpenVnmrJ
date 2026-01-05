
/*
  WARNING: THIS FILE IS AUTO-GENERATED. DO NOT MODIFY.

  This file was generated from Console_Conf.idl using "rtiddsgen".
  The rtiddsgen tool is part of the RTI Data Distribution Service distribution.
  For more information, type 'rtiddsgen -help' at a command shell
  or consult the RTI Data Distribution Service manual.
*/


#ifndef NDDS_STANDALONE_TYPE
    #ifdef __cplusplus
        #ifndef ndds_cpp_h
            #include "ndds/ndds_cpp.h"
        #endif
        #ifndef dds_c_log_impl_h              
            #include "dds_c/dds_c_log_impl.h"                                
        #endif        
    #else
        #ifndef ndds_c_h
            #include "ndds/ndds_c.h"
        #endif
    #endif
    
    #ifndef cdr_type_h
        #include "cdr/cdr_type.h"
    #endif    

    #ifndef osapi_heap_h
        #include "osapi/osapi_heap.h" 
    #endif
#else
    #include "ndds_standalone_type.h"
#endif



#ifndef Console_Conf_h
#include "Console_Conf.h"
#endif

/* ========================================================================= */
const char *Console_ConfTYPENAME = "Console_Conf";


/* --- Special Varian Inc Added Template Function  ----- */
extern DDS_ReturnCode_t Console_ConfTypeSupport_register_type(DDS_DomainParticipant*, const char*);
extern DDS_ReturnCode_t Console_ConfTypeSupport_create_data_ex(DDS_Boolean);

void getConsole_ConfInfo(NDDS_OBJ *myStruct)
{
    strcpy(myStruct->dataTypeName, Console_ConfTYPENAME);
    myStruct->TypeRegisterFunc = (DataTypeRegister) Console_ConfTypeSupport_register_type;
    myStruct->TypeAllocFunc = (DataTypeAllocate) Console_ConfTypeSupport_create_data_ex;
}
/* --- End Special Varian Inc Added Template Function  ----- */


    
    

RTIBool Console_Conf_initialize(
    Console_Conf* sample) {
    return Console_Conf_initialize_ex(sample,RTI_TRUE);
}
        
RTIBool Console_Conf_initialize_ex(
    Console_Conf* sample,RTIBool allocatePointers)
{

    if (!RTICdrType_initLong(&sample->structVersion)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->ConsoleTypeFlag)) {
        return RTI_FALSE;
    }                
            
    if (!RTICdrType_initLong(&sample->SystemRevId)) {
        return RTI_FALSE;
    }                
            
    sample->VxWorksVersion = DDS_String_alloc(((CONSOLE_CONF_MAX_STR_LEN)));
    if (sample->VxWorksVersion == NULL) {
        return RTI_FALSE;
    }
            
    sample->RtiNddsVersion = DDS_String_alloc(((CONSOLE_CONF_MAX_STR_LEN)));
    if (sample->RtiNddsVersion == NULL) {
        return RTI_FALSE;
    }
            
    sample->PsgInterpVersion = DDS_String_alloc(((CONSOLE_CONF_MAX_STR_LEN)));
    if (sample->PsgInterpVersion == NULL) {
        return RTI_FALSE;
    }
            
    sample->CompileDate = DDS_String_alloc(((CONSOLE_CONF_MAX_STR_LEN)));
    if (sample->CompileDate == NULL) {
        return RTI_FALSE;
    }
            
    sample->ddrmd5 = DDS_String_alloc(((CONSOLE_CONF_MAX_STR_LEN)));
    if (sample->ddrmd5 == NULL) {
        return RTI_FALSE;
    }
            
    sample->gradientmd5 = DDS_String_alloc(((CONSOLE_CONF_MAX_STR_LEN)));
    if (sample->gradientmd5 == NULL) {
        return RTI_FALSE;
    }
            
    sample->lockmd5 = DDS_String_alloc(((CONSOLE_CONF_MAX_STR_LEN)));
    if (sample->lockmd5 == NULL) {
        return RTI_FALSE;
    }
            
    sample->mastermd5 = DDS_String_alloc(((CONSOLE_CONF_MAX_STR_LEN)));
    if (sample->mastermd5 == NULL) {
        return RTI_FALSE;
    }
            
    sample->nvlibmd5 = DDS_String_alloc(((CONSOLE_CONF_MAX_STR_LEN)));
    if (sample->nvlibmd5 == NULL) {
        return RTI_FALSE;
    }
            
    sample->nvScriptmd5 = DDS_String_alloc(((CONSOLE_CONF_MAX_STR_LEN)));
    if (sample->nvScriptmd5 == NULL) {
        return RTI_FALSE;
    }
            
    sample->pfgmd5 = DDS_String_alloc(((CONSOLE_CONF_MAX_STR_LEN)));
    if (sample->pfgmd5 == NULL) {
        return RTI_FALSE;
    }
            
    sample->lpfgmd5 = DDS_String_alloc(((CONSOLE_CONF_MAX_STR_LEN)));
    if (sample->lpfgmd5 == NULL) {
        return RTI_FALSE;
    }
            
    sample->rfmd5 = DDS_String_alloc(((CONSOLE_CONF_MAX_STR_LEN)));
    if (sample->rfmd5 == NULL) {
        return RTI_FALSE;
    }
            
    sample->vxWorksKernelmd5 = DDS_String_alloc(((CONSOLE_CONF_MAX_STR_LEN)));
    if (sample->vxWorksKernelmd5 == NULL) {
        return RTI_FALSE;
    }
            
    sample->fpgaLoadStr = DDS_String_alloc(((CONSOLE_CONF_MAX_STR_LEN)));
    if (sample->fpgaLoadStr == NULL) {
        return RTI_FALSE;
    }
            

    return RTI_TRUE;
}

void Console_Conf_finalize(
    Console_Conf* sample)
{
    Console_Conf_finalize_ex(sample,RTI_TRUE);
}
        
void Console_Conf_finalize_ex(
    Console_Conf* sample,RTIBool deletePointers)
{

    DDS_String_free(sample->VxWorksVersion);                
            
    DDS_String_free(sample->RtiNddsVersion);                
            
    DDS_String_free(sample->PsgInterpVersion);                
            
    DDS_String_free(sample->CompileDate);                
            
    DDS_String_free(sample->ddrmd5);                
            
    DDS_String_free(sample->gradientmd5);                
            
    DDS_String_free(sample->lockmd5);                
            
    DDS_String_free(sample->mastermd5);                
            
    DDS_String_free(sample->nvlibmd5);                
            
    DDS_String_free(sample->nvScriptmd5);                
            
    DDS_String_free(sample->pfgmd5);                
            
    DDS_String_free(sample->lpfgmd5);                
            
    DDS_String_free(sample->rfmd5);                
            
    DDS_String_free(sample->vxWorksKernelmd5);                
            
    DDS_String_free(sample->fpgaLoadStr);                
            
}

RTIBool Console_Conf_copy(
    Console_Conf* dst,
    const Console_Conf* src)
{

    if (!RTICdrType_copyLong(
        &dst->structVersion, &src->structVersion)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->ConsoleTypeFlag, &src->ConsoleTypeFlag)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyLong(
        &dst->SystemRevId, &src->SystemRevId)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyString(
        dst->VxWorksVersion, src->VxWorksVersion, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyString(
        dst->RtiNddsVersion, src->RtiNddsVersion, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyString(
        dst->PsgInterpVersion, src->PsgInterpVersion, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyString(
        dst->CompileDate, src->CompileDate, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyString(
        dst->ddrmd5, src->ddrmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyString(
        dst->gradientmd5, src->gradientmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyString(
        dst->lockmd5, src->lockmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyString(
        dst->mastermd5, src->mastermd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyString(
        dst->nvlibmd5, src->nvlibmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyString(
        dst->nvScriptmd5, src->nvScriptmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyString(
        dst->pfgmd5, src->pfgmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyString(
        dst->lpfgmd5, src->lpfgmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyString(
        dst->rfmd5, src->rfmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyString(
        dst->vxWorksKernelmd5, src->vxWorksKernelmd5, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            
    if (!RTICdrType_copyString(
        dst->fpgaLoadStr, src->fpgaLoadStr, ((CONSOLE_CONF_MAX_STR_LEN)) + 1)) {
        return RTI_FALSE;
    }
            

    return RTI_TRUE;
}


/**
 * <<IMPLEMENTATION>>
 *
 * Defines:  TSeq, T
 *
 * Configure and implement 'Console_Conf' sequence class.
 */
#define T Console_Conf
#define TSeq Console_ConfSeq
#define T_initialize_ex Console_Conf_initialize_ex
#define T_finalize_ex   Console_Conf_finalize_ex
#define T_copy       Console_Conf_copy

#ifndef NDDS_STANDALONE_TYPE
#include "dds_c/generic/dds_c_sequence_TSeq.gen"
#ifdef __cplusplus
#include "dds_cpp/generic/dds_cpp_sequence_TSeq.gen"
#endif
#else
#include "dds_c_sequence_TSeq.gen"
#ifdef __cplusplus
#include "dds_cpp_sequence_TSeq.gen"
#endif
#endif

#undef T_copy
#undef T_finalize_ex
#undef T_initialize_ex
#undef TSeq
#undef T

