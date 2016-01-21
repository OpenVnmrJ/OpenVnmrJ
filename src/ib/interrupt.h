/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef INTERRUPT_H
#define INTERRUPT_H
/************************************************************************
*									
*
*************************************************************************
*									
*  Charly Gatot
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*									
*************************************************************************/



#include "msgprt.h"
#include "message.h"

/* This defines to use msgerr_print for interrupt message */
#define	interrupt_msg msgerr_print

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

void interrupt_register(Notify_client, int major, int minor, int signal);
void interrupt_begin();
int interrupt();
void interrupt_end();

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif INTERRUPT_H
