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
/*
 *    This gradient_fifo.h file is auto generated via Tau's CVS make base on the
 *    RF FPGA verilog source
 *    It's place in our SCCS for consistency with our software
 *    Greg Brissey 3/25/2004
 */

#ifndef GRADIENT_FIFO_H
#define GRADIENT_FIFO_H
#define encode_GRADIENTNoOp(W) ((0<<26)|((W&1)<<31))
#define encode_GRADIENTSetDuration(W,duration) ((1<<26)|((W&1)<<31)|((duration&((1<<(25-0+1))-1))<<0))
#define encode_GRADIENTSetRepeatedDuration(W,rep,duration) ((17<<26)|((W&1)<<31)|((rep&((1<<(25-10+1))-1))<<10)|((duration&((1<<(9-0+1))-1))<<0))
#define encode_GRADIENTSetXAmp(W,clear,count,xamp) ((2<<26)|((W&1)<<31)|((clear&1)<<25)|((count&((1<<(24-16+1))-1))<<16)|((xamp&((1<<(15-0+1))-1))<<0))
#define encode_GRADIENTSetYAmp(W,clear,count,yamp) ((3<<26)|((W&1)<<31)|((clear&1)<<25)|((count&((1<<(24-16+1))-1))<<16)|((yamp&((1<<(15-0+1))-1))<<0))
#define encode_GRADIENTSetZAmp(W,clear,count,zamp) ((4<<26)|((W&1)<<31)|((clear&1)<<25)|((count&((1<<(24-16+1))-1))<<16)|((zamp&((1<<(15-0+1))-1))<<0))
#define encode_GRADIENTSetB0Amp(W,clear,count,b0amp) ((5<<26)|((W&1)<<31)|((clear&1)<<25)|((count&((1<<(24-16+1))-1))<<16)|((b0amp&((1<<(15-0+1))-1))<<0))
#define encode_GRADIENTSetXAmpScale(W,clear,count,xamp_scale) ((6<<26)|((W&1)<<31)|((clear&1)<<25)|((count&((1<<(24-16+1))-1))<<16)|((xamp_scale&((1<<(15-0+1))-1))<<0))
#define encode_GRADIENTSetYAmpScale(W,clear,count,yamp_scale) ((7<<26)|((W&1)<<31)|((clear&1)<<25)|((count&((1<<(24-16+1))-1))<<16)|((yamp_scale&((1<<(15-0+1))-1))<<0))
#define encode_GRADIENTSetZAmpScale(W,clear,count,zamp_scale) ((8<<26)|((W&1)<<31)|((clear&1)<<25)|((count&((1<<(24-16+1))-1))<<16)|((zamp_scale&((1<<(15-0+1))-1))<<0))
#define encode_GRADIENTSetB0AmpScale(W,clear,count,b0amp_scale) ((9<<26)|((W&1)<<31)|((clear&1)<<25)|((count&((1<<(24-16+1))-1))<<16)|((b0amp_scale&((1<<(15-0+1))-1))<<0))
#define encode_GRADIENTSetXEcc(W,xecc) ((10<<26)|((W&1)<<31)|((xecc&((1<<(15-0+1))-1))<<0))
#define encode_GRADIENTSetYEcc(W,yecc) ((11<<26)|((W&1)<<31)|((yecc&((1<<(15-0+1))-1))<<0))
#define encode_GRADIENTSetZEcc(W,zecc) ((12<<26)|((W&1)<<31)|((zecc&((1<<(15-0+1))-1))<<0))
#define encode_GRADIENTSetB0Ecc(W,b0ecc) ((13<<26)|((W&1)<<31)|((b0ecc&((1<<(15-0+1))-1))<<0))
#define encode_GRADIENTSetShim(W,shim) ((14<<26)|((W&1)<<31)|((shim&((1<<(19-0+1))-1))<<0))
#define encode_GRADIENTSetGates(W,mask,gates) ((15<<26)|((W&1)<<31)|((mask&((1<<(23-12+1))-1))<<12)|((gates&((1<<(11-0+1))-1))<<0))
#define encode_GRADIENTSetUser(W,user) ((16<<26)|((W&1)<<31)|((user&((1<<(15-0+1))-1))<<0))
#endif
