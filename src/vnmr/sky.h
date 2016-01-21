/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef SKY_H
#define SKY_H

extern void vvvrmult(float *in1, int is1, float *in2, int is2, float *out1, int os1, int n);
extern void datafill(float *buffer, int n, float value);
extern void skymax(float *in1, float *in2, float *out, int n);
extern void skyadd(float *in1, float *in2, float *out, int n);
extern void vrsum(float *in1, int is1, float *sum, int npnt);
extern void vssubr(float *inptr, float val, float *outptr, int n);
extern void vvrramp(float *in1, int is1, float *out1, int os1, float start, float delta, int npnt);
extern void vvvcmult(void *in1, int is1, void *in2, int is2, void *out1, int os1, int npoints);
extern void vvvhcmult(void *in1, int is1, void *in2, int is2, void *out1, int os1, int npoints);
extern void scfix1(float *frompntr, int fromincr, float mult, short *topntr, int toincr, int npnts);
extern void scabs(float *frompntr, int fromincr, float mult, short *topntr, int toincr, int npnts, int sgn);
extern void preproc(float *datapntr, int n, int datatype);
extern void postproc(float *datapntr, int n,  int datatype);
extern void combine(float *combinebuf, float *outp, int npoints, int datatype,
                    float r1, float r2, float r3, float r4);
extern void shiftComplexData(float *ptr, int shiftpts, int npoints, int len);
extern void negateimaginary(float *data, int npoints, int datatype);
extern void cnvrts32(float scalefactor, int *inp, float *outp, int npx, int lsfidx);
extern void cnvrts16(float scalefactor, short *inp, float *outp, int npx, int lsfidx);

#endif 
