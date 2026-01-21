
/*
  WARNING: THIS FILE IS AUTO-GENERATED. DO NOT MODIFY.

  This file was generated from Monitor_Cmd.idl using "rtiddsgen".
  The rtiddsgen tool is part of the RTI Data Distribution Service distribution.
  For more information, type 'rtiddsgen -help' at a command shell
  or consult the RTI Data Distribution Service manual.
*/

#ifndef Monitor_CmdSupport_h
#define Monitor_CmdSupport_h

/* Uses */
#ifndef Monitor_Cmd_h
#include "Monitor_Cmd.h"
#endif



#ifdef __cplusplus
#ifndef ndds_cpp_h
  #include "ndds/ndds_cpp.h"
#endif
#else
#ifndef ndds_c_h
  #include "ndds/ndds_c.h"
#endif
#endif

        

/* ========================================================================= */
/**
   Uses:     T

   Defines:  TTypeSupport, TDataWriter, TDataReader

   Organized using the well-documented "Generics Pattern" for
   implementing generics in C and C++.
*/

#if (defined(RTI_WIN32) || defined (RTI_WINCE)) && defined(NDDS_USER_DLL_EXPORT)
  /* If the code is building on Windows, start exporting symbols.
   */
  #undef NDDSUSERDllExport
  #define NDDSUSERDllExport __declspec(dllexport)

#ifdef __cplusplus
  /* If we're building on Windows, explicitly import the superclasses of
   * the types declared below.
   */        
  class __declspec(dllimport) DDSTypeSupport;
  class __declspec(dllimport) DDSDataWriter;
  class __declspec(dllimport) DDSDataReader;
#endif

#endif

#ifdef __cplusplus

DDS_TYPESUPPORT_CPP(Monitor_CmdTypeSupport, Monitor_Cmd);

DDS_DATAWRITER_CPP(Monitor_CmdDataWriter, Monitor_Cmd);
DDS_DATAREADER_CPP(Monitor_CmdDataReader, Monitor_CmdSeq, Monitor_Cmd);


#else

DDS_TYPESUPPORT_C(Monitor_CmdTypeSupport, Monitor_Cmd);
DDS_DATAWRITER_C(Monitor_CmdDataWriter, Monitor_Cmd);
DDS_DATAREADER_C(Monitor_CmdDataReader, Monitor_CmdSeq, Monitor_Cmd);

#endif

#if (defined(RTI_WIN32) || defined (RTI_WINCE)) && defined(NDDS_USER_DLL_EXPORT)
  /* If the code is building on Windows, stop exporting symbols.
   */
  #undef NDDSUSERDllExport
  #define NDDSUSERDllExport
#endif



#endif	/* Monitor_CmdSupport_h */
