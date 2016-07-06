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
#ifndef fpgaBaseISR_header

#define fpgaBaseISR_header

#ifndef mask_value
  #define mask_value(fpga,field,value) \
    (((value)>> fpga##_##field##_pos) & (((fpga##_##field##_width<32) ? (1<<fpga##_##field##_width):0)-1))
#endif

#define NO_MASKING 0xffffffff

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif


/* --------- ANSI/C++ compliant function prototypes --------------- */
 
#if defined(__STDC__) || defined(__cplusplus)

extern void fpgaBaseISRs(size_t slice, int arg); 
extern void initFpgaBaseISR(void);
extern size_t initFpgaSliceISR(volatile unsigned *status,  volatile unsigned *enable, volatile unsigned *clear);
extern size_t initFpgaBaseISRs( VOIDFUNCPTR routine, volatile unsigned *status, volatile unsigned *enable, volatile unsigned *clear);
extern STATUS fpgaIntConnect ( VOIDFUNCPTR   routine, int parameter, unsigned int mask);
extern STATUS fpgaIntRemove ( VOIDFUNCPTR   routine,  int parameter );
extern STATUS fpgaIntChngMask ( VOIDFUNCPTR   routine, int parameter, unsigned int  newmask);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */
 
size_t initFpgaSliceISR(volatile unsigned *status, volatile unsigned *enable, volatile unsigned *clear);
extern void fpgaInitBaseISR();
extern STATUS fpgaIntConnect();

#endif
 
#ifdef __cplusplus
}
#endif
 
#endif
