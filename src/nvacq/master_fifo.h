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
 *    This master_fifo.h file is auto generated via Tau's CVS make base on the
 *    RF FPGA verilog source
 *    It's place in our SCCS for consistency with our software
 *    Greg Brissey 3/25/2004
 */

#ifndef MASTER_FIFO_H
#define MASTER_FIFO_H
#define encode_MASTERSetDuration(W,duration) ((1<<26)|((W&1)<<31)|((duration&((1<<(25-0+1))-1))<<0))
#define encode_MASTERSetGates(W,mask,gates) ((2<<26)|((W&1)<<31)|((mask&((1<<(23-12+1))-1))<<12)|((gates&((1<<(11-0+1))-1))<<0))
#define encode_MASTERSelectTimeBase(W,select) ((3<<26)|((W&1)<<31)|((select&1)<<0))
#define encode_MASTERSetSPI(W,spi_select,chip_select,data) ((1<<30)|((W&1)<<31)|((spi_select&((1<<(29-28+1))-1))<<28)|((chip_select&((1<<(27-24+1))-1))<<24)|((data&((1<<(23-0+1))-1))<<0))
#define encode_MASTERSetAux(W,set_phase,address,data) ((9<<26)|((W&1)<<31)|((set_phase&1)<<12)|((address&((1<<(11-8+1))-1))<<8)|((data&((1<<(7-0+1))-1))<<0))
#define encode_MASTERNoOp(W) ((0<<26)|((W&1)<<31))
#endif
