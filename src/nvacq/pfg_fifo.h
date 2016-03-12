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
 *    This pfg_fifo.h file is auto generated via Tau's CVS make base on the
 *    RF FPGA verilog source
 *    It's place in our SCCS for consistency with our software
 *    Greg Brissey 3/25/2004
 */

#ifndef PFG_FIFO_H
#define PFG_FIFO_H
#define encode_PFGNoOp(W) ((0<<26)|((W&1)<<31))
#define encode_PFGSetDuration(W,duration) ((1<<26)|((W&1)<<31)|((duration&((1<<(25-0+1))-1))<<0))
#define encode_PFGSetXAmp(W,clear,count,xamp) ((2<<26)|((W&1)<<31)|((clear&1)<<25)|((count&((1<<(24-16+1))-1))<<16)|((xamp&((1<<(15-0+1))-1))<<0))
#define encode_PFGSetYAmp(W,clear,count,yamp) ((3<<26)|((W&1)<<31)|((clear&1)<<25)|((count&((1<<(24-16+1))-1))<<16)|((yamp&((1<<(15-0+1))-1))<<0))
#define encode_PFGSetZAmp(W,clear,count,zamp) ((4<<26)|((W&1)<<31)|((clear&1)<<25)|((count&((1<<(24-16+1))-1))<<16)|((zamp&((1<<(15-0+1))-1))<<0))
#define encode_PFGSetXAmpScale(W,clear,count,xamp_scale) ((5<<26)|((W&1)<<31)|((clear&1)<<25)|((count&((1<<(24-16+1))-1))<<16)|((xamp_scale&((1<<(15-0+1))-1))<<0))
#define encode_PFGSetYAmpScale(W,clear,count,yamp_scale) ((6<<26)|((W&1)<<31)|((clear&1)<<25)|((count&((1<<(24-16+1))-1))<<16)|((yamp_scale&((1<<(15-0+1))-1))<<0))
#define encode_PFGSetZAmpScale(W,clear,count,zamp_scale) ((7<<26)|((W&1)<<31)|((clear&1)<<25)|((count&((1<<(24-16+1))-1))<<16)|((zamp_scale&((1<<(15-0+1))-1))<<0))
#define encode_PFGSetGates(W,mask,gates) ((8<<26)|((W&1)<<31)|((mask&((1<<(23-12+1))-1))<<12)|((gates&((1<<(11-0+1))-1))<<0))
#define encode_PFGSetUser(W,user) ((9<<26)|((W&1)<<31)|((user&((1<<(15-0+1))-1))<<0))
#endif
