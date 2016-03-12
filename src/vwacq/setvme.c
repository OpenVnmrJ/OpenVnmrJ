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

setVmeAccess()
{
     /********************************************************************
     *  VARIAN   Modification
     * Now That the setup completed
     * we alter TRANSPARET Transation Register of the 68040 to allow
     *   VME access from $10000000 -> $5fffffff address space
     *   we do this instead of programing the sysPhysMemDesc[] struct
     *   for the MMU. (Doing this way speeds up bootup
     *
     *  bits:
     * 31-24: logical Address base 
     * 23-16: logical address mask
     *    15: Enable Translation 
     * 14-13: Address mode (00-user,01-Super, 1X-any)
     *   6-5: cache mode: 00-cache wr/through, 01-cache copyback,
     *		     10-noncache,serialized, 11-noncache
     *     2: Write - 0 R/W, 1 R_Only
     *
     * 0xC040 - Enable, ignore FC2, non-cachable-serialized, Writable
     * #define DTT0_VALUE   0x10EfC040  from $10000000 -> $5fffffff
     *
     *  asm ("movec %0,dtt0" : : "r" (DTT0_VALUE));
     * asm ("movec %0,dtt1" : : "r" (DTT1_VALUE));
     *
     *  compile for mc68040, otherwise compiler errors will result
     */
#define DTT0_VALUE   0x10EFC040  /* from $10000000 -> $5fffffff */
#define DTT1_VALUE   0x20DFC040  /* from $20000000 -> $2fffffff */
     asm ("movec %0,dtt0" : : "r" (DTT0_VALUE));
     asm ("movec %0,dtt1" : : "r" (DTT1_VALUE));
}
