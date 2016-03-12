/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef __FPGA_H
#define __FPGA_H

#ifndef mask_value
  #define mask_value(fpga,field,value) \
    (((value)>> fpga##_##field##_pos) & (((fpga##_##field##_width<32) ? (1<<fpga##_##field##_width):0)-1))
#endif

#ifndef  get_mask
#define get_mask(fpga,field) \
     ((((((fpga##_##field##_width<32)?(1<<fpga##_##field##_width):0)-1))<<fpga##_##field##_pos))
#endif

#ifndef set_field_value
  #define set_field_value(fpga,field,value) \
    ((value << fpga##_##field##_pos))
#endif
#ifndef get_register
  #define get_register(fpga,field) \
    (*((volatile unsigned *)(fpga##_BASE + fpga##_##field)))
#endif
#ifndef set_register
  #define set_register(fpga,field,value) \
    ((*((volatile unsigned *)(fpga##_BASE + fpga##_##field))) = value)
#endif
#ifndef get_pointer
  #define get_pointer(fpga,field) \
    ((volatile unsigned *)(fpga##_BASE + fpga##_##field))
#endif
#ifndef get_instrfifosize
  #define get_instrfifosize(fpga) \
    ( ( 1 << fpga##_instruction_fifo_count_width ) )
#endif
#ifndef get_field
  #define __get_field(fpga,field) \
    ((*((volatile unsigned *)(fpga##_BASE + fpga##_##field##_addr))>> \
    fpga##_##field##_pos) & (((fpga##_##field##_width<32) ? (1<<fpga##_##field##_width):0)-1))
  #define get_field(fpga,field) __get_field(fpga,field)
#endif
#ifndef set_field
  #define __set_field(fpga,field,value) \
    ((*((volatile unsigned *)(fpga##_BASE + fpga##_##field##_addr))) = \
    (*((volatile unsigned *)(fpga##_BASE + fpga##_##field##_addr))) & \
    ~((((fpga##_##field##_width<32)?(1<<fpga##_##field##_width):0)-1)<<fpga##_##field##_pos) | \
    ((value&(((fpga##_##field##_width<32)?(1<<fpga##_##field##_width):0)-1))<<fpga##_##field##_pos))
  #define set_field(fpga,field,value) __set_field(fpga,field,value)
#endif

#endif /* __FPGA_H */
