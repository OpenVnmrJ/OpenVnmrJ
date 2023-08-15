/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef SYNTHESIZER_H
#define SYNTHESIZER_H

typedef struct _SynthesizerType {
  int ptsval;
  const char * type;
  double fmin, fmax, fstep;
  void * bp;
} SynthesizerType;

#ifdef __cplusplus
class Synthesizer
{
 public:
  
 private:
  // UNUSED
  int *fliststore;  // this member never referred to anywhere
  int calcFTW1(double freq);
  int calcFTW2(double freq);
  long long getFTW(double freq);
  long long getFTW1(double freq);
  
 public:
  const char* synT;
  Synthesizer(int slot);
  const SynthesizerType* type;
  int encodeFreq(double freq,int *array);
};
#else
extern SynthesizerType* synthType(int);
#endif /* __cplusplus */
#endif
