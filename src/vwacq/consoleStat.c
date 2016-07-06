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

#include <vxWorks.h>

#include "hostAcqStructs.h"
#include "consoleStat.h"

extern STATUS_BLOCK	currentStatBlock;


void
setLockLevel( int newLockLevel )
{
	currentStatBlock.stb.AcqLockLevel = (short) newLockLevel;
}

void
setSpinAct( int newSpinAct )
{
	currentStatBlock.stb.AcqSpinAct = (short) newSpinAct;
}

void
setSpinSet( int newSpinSet )
{
	currentStatBlock.stb.AcqSpinSet = (short) newSpinSet;
}

int
getSpinSet()
{
	return((int) currentStatBlock.stb.AcqSpinSet );
}

short
getLSDVword()
{
	return( currentStatBlock.stb.AcqLSDVbits );
}

void
setLSDVword( ushort newLSDVword )
{
	currentStatBlock.stb.AcqLSDVbits = newLSDVword;
}

void
setLSDVbits( ushort bits2set )
{
	currentStatBlock.stb.AcqLSDVbits |= bits2set;
}

void
clearLSDVbits( ushort bits2clear )
{
	currentStatBlock.stb.AcqLSDVbits &= ~(bits2clear);
}
