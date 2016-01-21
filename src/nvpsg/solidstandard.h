/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef SOLIDSTANDARD_H
#define SOLIDSTANDARD_H

#include <standard.h>
#include <Pbox_psg.h> 
#include "soliddefs.h"      //Structures and Definitions of Constant
#include "solidelements.h"  //Miscellaneous Programs Including getname()
#ifdef NVPSG
#include "solidchoppers.h"  // Replacement choppers use userDECShape
#endif
#include "solidshapegen.h"  //Functions to Calculate and Run .DEC files 
#include "soliddecshapes.h" //All Software for DSEQ, SPINAL and TPPM
#include "solidobjects.h"   //Non-waveform pulse sequence functions.
#include "solidmpseqs.h"    //"get" Functions for Multiple Pulse Sequences
#include "solidstates.h"    //Functions to Output STATE's for Shaped Pulses
#include "solidpulses.h"    //"get" Functions for Shaped Pulses
#include "solidwshapes.h"   //Functions to run shapes with acquisition windows
#include "soliddutycycle.h" //Functions to calculate the dutycycle
#include "solidhhdec.h"     //Functions to support Ames contribution.

#endif


