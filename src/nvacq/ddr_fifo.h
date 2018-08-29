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
 *    This ddr_fifo.h file is auto generated via Tau's CVS make base on the
 *    RF FPGA verilog source
 *    It's place in our SCCS for consistency with our software
 *    Greg Brissey 3/25/2004
 */
#ifndef DDR_FIFO_H
#define DDR_FIFO_H
#define encode_DDRSetDuration(W,duration) ((1<<26)|((W&1)<<31)|((duration&((1<<(25-0+1))-1))<<0))
#define encode_DDRSetGates(W,mask,gates) ((2<<26)|((W&1)<<31)|((mask&((1<<(23-12+1))-1))<<12)|((gates&((1<<(11-0+1))-1))<<0))
#define encode_DDRSetGain(W,gain) ((3<<26)|((W&1)<<31)|((gain&((1<<(4-0+1))-1))<<0))
#define encode_DDRSetAux(W,address,data) ((9<<26)|((W&1)<<31)|((address&((1<<(12-8+1))-1))<<8)|((data&((1<<(7-0+1))-1))<<0))
#define encode_DDRNoOp(W) ((0<<26)|((W&1)<<31))
#endif
