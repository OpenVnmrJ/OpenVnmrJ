/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "ACQ_SUN.h"
#include "acodes.h"
#include "acqparms.h"
#include "macros.h"

S_sli(address, mode, value)
/*
 *	Sets the SLI (Synchonous Line Interface) board outputs to a
 *	selected value.
 *
 *	"address" is the address of the SLI board in the form:
 *		(chip_address * 64) + register_base
 *	The chip_address is "9" if the SLI is in the analog side of the
 *	VME card cage, and "C" (hex) if it is in the digital side.
 *	The register_base is normally set to "90" (hex) in either case.
 *	These values are selected by jumper J7R of the SLI board.
 *	Therefore, the value of "address" will normally be:
 *		C90 (hex) = 3216 (dec) (digital (left) side), or
 *		990 (hex) = 2448 (dec) (analog (right) side).
 *
 *	"mode" tells how to combine the given value with the current SLI
 *	output to produce the new output value.
 *		mode = SLI_SET: Load the new value directly into the SLI.
 *		mode = SLI_OR:  Logically OR the new value with the old.
 *		mode = SLI_AND: Logically AND the new value with the old.
 *		mode = SLI_XOR: Logically XOR the new value with the old.
 *
 *	"value" is the new value.
 *
 *	The pulse sequence delay time is 10.2us per call (5 AP bus cycles
 *	and a 200ns delay).
*/
int address;
int mode;
unsigned value;
{
    char mess[MAXSTR];

    validate_imaging_config("sli");

    /* Check for valid mode */
    switch (mode){
      case SLI_SET:
      case SLI_OR:
      case SLI_AND:
      case SLI_XOR:
	break;
      default:
	sprintf(mess,"sli(): Illegal mode: %d", mode);
	text_error(mess);
	psg_abort(1);
	break;
    }

    putcode( (c68int)SLI );
    putcode( (c68int)address );
    putcode( (c68int)mode );
    putcode( (c68int)((value>>16) & 0xffff) );
    putcode( (c68int)(value & 0xffff) );
}

S_vsli(address, mode, var)
/*
/*	Sets the SLI (Synchonous Line Interface) board outputs to the value
/*	determined by selected real-time variables.
/*
/*	"address" is the address of the SLI board in the form:
/*		(chip_address * 64) + register_base
/*	The chip_address is "9" if the SLI is in the analog side of the
/*	VME card cage, and "C" (hex) if it is in the digital side.
/*	The register_base is normally set to "90" (hex) in either case.
/*	These values are selected by jumper J7R of the SLI board.
/*	Therefore, the value of "address" will normally be:
/*		C90 (hex) = 3216 (dec) (digital (left) side), or
/*		990 (hex) = 2448 (dec) (analog (right) side).
/*
/*	"mode" tells how to combine the real-time variables' values with
/*	the current SLI output to produce the new output value.
/*		mode = SLI_SET: Load the new value directly into the SLI.
/*		mode = SLI_OR:  Logically OR the new value with the old.
/*		mode = SLI_AND: Logically AND the new value with the old.
/*		mode = SLI_XOR: Logically XOR the new value with the old.
/*
/*	"var" determines the real-time variables containing the new value.
/*	Since the value is 32 bits, two variables are actually used. The
/*	specified variable holds the high word of the value, and the next
/*	variable holds the low word.  Therefore, "var" must be between
/*	v1 and v13; v14 cannot be specified.
/*
/*	The pulse sequence delay time is 10.2us per call (5 AP bus cycles
/*	and a 200ns delay).
*/
int address;
int mode;
codeint var;
{
    char mess[MAXSTR];

    validate_imaging_config("vsli");

    /* Test for valid "var" */
    if ( (var < v1) || (var > v13) ){
	sprintf(mess,"vsli(): Illegal real-time variable: %d", var);
	text_error(mess);
	psg_abort(1);
    }

    /* Check for valid mode */
    switch (mode){
      case SLI_SET:
      case SLI_OR:
      case SLI_AND:
      case SLI_XOR:
	break;
      default:
	sprintf(mess,"vsli(): Illegal mode: %d", mode);
	text_error(mess);
	psg_abort(1);
	break;
    }

    putcode( (c68int)VSLI );
    putcode( (c68int)address );
    putcode( (c68int)mode );
    putcode( (c68int)var );
}
