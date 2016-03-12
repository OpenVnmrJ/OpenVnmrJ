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
 *    This rf_fifo.h file is auto generated via Tau's CVS make base on the
 *    RF FPGA verilog source
 *    It's place in our SCCS for consistency with our software
 *    Greg Brissey 7/20/2004
 */
#ifndef RF_FIFO_H
#define RF_FIFO_H
#define encode_RFSetDuration(W,duration) ((1<<26)|((W&1)<<31)|((duration&((1<<(25-0+1))-1))<<0))
#define encode_RFSetGates(W,mask,gates) ((2<<26)|((W&1)<<31)|((mask&((1<<(23-12+1))-1))<<12)|((gates&((1<<(11-0+1))-1))<<0))
#define encode_RFSetPhase(W,clear,count,phase) ((4<<26)|((W&1)<<31)|((clear&1)<<25)|((count&((1<<(24-16+1))-1))<<16)|((phase&((1<<(15-0+1))-1))<<0))
#define encode_RFSetPhaseC(W,clear,count,phase) ((5<<26)|((W&1)<<31)|((clear&1)<<25)|((count&((1<<(24-16+1))-1))<<16)|((phase&((1<<(15-0+1))-1))<<0))
#define encode_RFSetAmp(W,clear,count,amplitude) ((6<<26)|((W&1)<<31)|((clear&1)<<25)|((count&((1<<(24-16+1))-1))<<16)|((amplitude&((1<<(15-0+1))-1))<<0))
#define encode_RFSetAmpScale(W,clear,count,amplitude_scale) ((7<<26)|((W&1)<<31)|((clear&1)<<25)|((count&((1<<(24-16+1))-1))<<16)|((amplitude_scale&((1<<(15-0+1))-1))<<0))
#define encode_RFSetUser(W,user) ((8<<26)|((W&1)<<31)|((user&((1<<(2-0+1))-1))<<0))
#define encode_RFSetAux(W,address,data) ((9<<26)|((W&1)<<31)|((address&((1<<(11-8+1))-1))<<8)|((data&((1<<(7-0+1))-1))<<0))
#define encode_RFNoOp(W) ((0<<26)|((W&1)<<31))
#endif
