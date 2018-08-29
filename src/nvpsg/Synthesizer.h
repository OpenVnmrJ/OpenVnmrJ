/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef _SYNTHESIZER_H
#define _SYNTHESIZER_H

class Synthesizer
{
 public:
  struct SynthesizerType {
    int ptsval;
    const char * type;
    double fmin, fmax, fstep;
    void * bp;
  };
  
 private:
  const char* name;
  int *fliststore;
  int calcFTW1(double freq);
  int calcFTW2(double freq);
  long long getFTW(double freq);
  long long getFTW1(double freq);

 public:
  static int dds2_ftw1(double freq);
  static int dds2_ftw2(double freq);
  static int dds_ftw1(double freq);
  static int dds_ftw2(double freq);
  static unsigned dds2_switch(double freq);
  static unsigned dds_switch(double freq);

  int format_dds(int *buffer, 
		 unsigned long ftw1, unsigned long ftw2, unsigned long rfswitch,
		 int atten, unsigned short freq);

  int format_var400(int *buffer, unsigned long long ftw, int rfswitch);

 public:
  const char* synT;
  const SynthesizerType* type;
  int encodeFreq(double freq,int *array);
  int adviseFreq();
  Synthesizer(const char* name, int slot);
};

#endif /* _SYNTHESIZER_H */
