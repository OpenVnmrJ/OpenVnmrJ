/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCserialShimsh
#define INCserialShimsh

#define  NO_SHIMS	0
#define  QSPI_SHIMS	1
#define  SERIAL_SHIMS	2
#define  RRI_SHIMS	3
#define  MSR_SERIAL_SHIMS 4
#define  MSR_RRI_SHIMS	5
#define  OMT_SHIMS	6
#define  MSR_OMT_SHIMS	7
#define  APBUS_SHIMS	8
#define  SPI_SHIMS	9
#define  SPI_M_SHIMS	10
#define  SPI_THIN_SHIMS	11

#define  SHIMS_ON_MSR   1
#define  SHIMS_ON_162   2

extern  int	shimType;
#ifndef STATIC_SHIMSET_PARM   
extern  int	shimSet;
#endif
extern  int	shimLoc;
extern const int *qspi_dac_addr;


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern int determineShimType();
extern int setTimShim( int port, int board, int dac, int value, int prtmod );
extern int initSerialShims();
extern int readSerialShimType();
extern int setSerialShim( int dac, int value );
#ifdef MERCURY
extern int setRRIShim( int dac, int value);
#else
extern int setRRIShim( int dac, int value, int fifoFlag );
#endif
extern const int *shimGetQspiTab();
extern const int *shimGetImgQspiTab();

/* --------- NON-ANSI/C++ prototypes ------------  */
#else
 
extern int determineShimType();
extern int setTimShim();
extern int initSerialShims();
extern int readSerialShimType();
extern int setSerialShim();
extern int setRRIShim();
extern int *shimGetQspiTab();
extern int *shimGetImgQspiTab();

#endif  /* __STDC__ */
 
#ifdef __cplusplus
}
#endif

#endif /* INCrngBlkLibh */
