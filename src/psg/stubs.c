/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdarg.h>

int	Tflag;
int	interuption;
int	active;

#ifdef PSG_LC
int	Dflag;
#endif

/*  Dummy Routines of wjunk.c, SCCS category VNMR  */

void Wscrprintf()
{
}

void Winfoprintf()
{
}

void WerrprintfWithPos()
{
}

void Wprintfpos()
{
}

/*VARARGS0*/
void Werrprintf(char *format, ...)
{  char         str[1024];
   va_list      vargs;

   va_start(vargs, format);
   vsprintf(str,format,vargs);
   fprintf(stderr,"%s\n",str);
   va_end(vargs);
}

void Wistek4x05()
{}

void Wistek4x07()
{}


#ifdef MACOS

int is_newacq()
{
  return(1);
}

void sendExpproc() {}
void setup_comm() {}
void deliverMessageSuid() {}
void G_Delay() {}
void G_Pulse() {}
void pulse_phase_type() {}
void SetRFChanAttr() {}
void sliderfile() {}
void SetAPBit() {}
void rfchan_getampband() {}
void phase_var_type() {}
void APBit_Device() {}
void AP_Device() {}
void Attn_Device() {}
void RFChan_Device() {}
void SetAPAttr() {}
void SetAttnAttr() {}
void SetEventAttr() {}
void sync_on_event() {}
void delayer() {}
void rfchan_getpwrf() {}
void mClose() {}
void mOpen() {}
void rfchan_getbasefreq() {}
void rfchan_getrfband() {}
void G_Offset() {}
void offset() {}
void init_spare_offset() {}
void set_spare_freq() {}
void G_Simpulse() {}
void zgradpulse() {}
void setreceiver() {}
void G_RTDelay() {}
void G_Power() {}


#endif
