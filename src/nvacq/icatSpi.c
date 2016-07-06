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

#include <unistd.h>
#include <vxWorks.h>
#include "icatSpi.h"
#include "icatSpiFifo.h"
#include "ficl.h"
#include "rf.h"
#include "nvhardware.h"
#include "icat.h"
#include <semLib.h>

#define icat_spi_addr_len ICAT_SPI_ADDRESS_WIDTH
#define icat_spi_rw_len 1
#define icat_spi_data_len ICAT_SPI_DATA_WIDTH
#define icat_spi_word_len (icat_spi_addr_len + icat_spi_rw_len + icat_spi_data_len)
#define icat_spi_word(addr, data, rw) \
  ((((data)&((1<<icat_spi_data_len)-1)) << (icat_spi_addr_len+icat_spi_rw_len)) | \
   (((rw)  &((1<<icat_spi_rw_len)  -1)) << icat_spi_addr_len) |		\
   ((addr)&((1<<icat_spi_addr_len)-1)))

#define ad9747_spi_addr_len 5
#define ad9747_spi_pad_len 2
#define ad9747_spi_rw_len 1
#define ad9747_spi_data_len 8
#define ad9747_spi_word_len (ad9747_spi_addr_len + ad9747_spi_pad_len + ad9747_spi_rw_len + ad9747_spi_data_len)
#define ad9747_spi_word(addr,data,rw) \
  ((((~rw)&((1<<ad9747_spi_rw_len)-1)) << (ad9747_spi_data_len + ad9747_spi_addr_len + ad9747_spi_pad_len)) | \
   (((addr)&((1<<ad9747_spi_addr_len)-1)) << (ad9747_spi_data_len)) | \
   ((data)&((1<<ad9747_spi_data_len)-1)))

#define xmit_spi_wr ((volatile unsigned *)(FPGA_BASE_ADR+RF_xmit_spi_data_out_addr))
#define xmit_spi_rd ((volatile unsigned *)(FPGA_BASE_ADR+RF_xmit_spi_data_in_addr))

static int debug = 0;

static void clk_out(unsigned clk, unsigned frame, unsigned value)
{
  unsigned out = 0; 
  out |= icat_set_field(RF, xmit_spi_frame, frame);
  out |= icat_set_field(RF, xmit_spi_clk, clk);
  out |= icat_set_field(RF, xmit_spi_data_out, value);
  if (debug>10) printf("clk_out(frame=%d clk=%d data=%d)\n", frame, clk, value);
  *xmit_spi_wr = out;
}

#define ICAT_REG_SELECT (0)
#define ICAT_ISF_SELECT (1)
#define ICAT_DAC_SELECT (2)
#define ICAT_NONE_SELECT (-1)

static SEM_ID select_sem;
static int select_sem_initialized = 0;
static int spi_current_select = ICAT_NONE_SELECT;
static void xmit_spi_select(int select)
{
  if (!select_sem_initialized)
    {
      select_sem = semBCreate(SEM_Q_FIFO,SEM_FULL);
      select_sem_initialized = 1;
    }
  semTake(select_sem, WAIT_FOREVER);
  spi_current_select = select;
  /* LSB first */
  clk_out(0, 1, select&0x1);	/* clock low, frame high */
  clk_out(1, 1, select&0x1);
  clk_out(0, 1, (select&0x2)>>1);
  clk_out(1, 1, (select&0x2)>>1);
  clk_out(0, 0, 0);
}

static void xmit_spi_release(void)
{
  spi_current_select = ICAT_NONE_SELECT;
  semGive(select_sem);
}

/* ================================================================= */

/*
 * ISFlash base routines
 * The ISFlash routines are MSBit first 
 */

/*
 * Bring the CSB Low on a rising Clk edge to Begin
 * a transmission on the SPI to the ISF
 * Greg Brissey
 */
void spi_isf_csb_low()
{
  /* begin spi transaction on ISF */
  /* CSB driven low on CLK high */
  xmit_spi_select(ICAT_ISF_SELECT);
  clk_out(1, 1, 1);   // clock high, frame high (aka CSB low) 
}

/*
 * Bring the CSB High on a falling Clk edge to End
 * a transmission on the SPI to the ISF
 * Greg Brissey
 */
void spi_isf_csb_high()
{
  /* end transaction on SPI for ISF */
  /* drive CSB high on falling edge of CLK */
  clk_out(1, 1, 0);   // CLK high, CSB low
  clk_out(0, 0, 0);   // CLK low, CSB high
  xmit_spi_release();
}


/*
 * read back ISF bit 
 */
static unsigned spi_isf_recv_bit()
{
  unsigned data = *xmit_spi_rd;
  unsigned in   = icat_get_field(RF, xmit_spi_data_in, data);
  return in&0x1;
}

/*
 * send up to 32 bits to the ISFLash via the SPI
 */
unsigned spi_isf_send(unsigned value, int bits)
{
  unsigned bit_recv = 0; 
  unsigned bit_value; 
  unsigned value_recv = 0; 
  int i; 

 for(i=bits-1; i >= 0; i--)
  {
     /* MSB first */
     bit_value = value >> i;
     clk_out(0, 1, bit_value&0x1);	/* clock low, frame high */
     bit_recv = spi_isf_recv_bit();	/* always read */
     clk_out(1, 1, bit_value&0x1);	/* clock high, frame high */
     bit_recv = spi_isf_recv_bit();	/* always read */
     value_recv = value_recv << 1;  // here for one effective less shift
     value_recv = value_recv | (bit_recv & 0x1);
  }

  return value_recv;
}

/*
 * Obtain the ISFlash SPI status register value
 */
int spi_isf_status()
{
  int statuscmd = 0xD7;
  int ret_val;
  /* Flag Begin transmission first */
  spi_isf_csb_low();

  // issue Cmd to obtain status value
  ret_val = spi_isf_send(statuscmd, 8);

  // clock in 8 bits to get the 8-bit status value
  ret_val = spi_isf_send(0xFF, 8);

  IPRINT1(0,"\nStatus: 0x%x\n\n", ret_val & 0xFF );

  spi_isf_csb_high(); // end of transaction
  return ret_val & 0xFF;
}

//
// wait until ISFlash command is completed 
// It is important for Erasing and Writing to the flash
// to wait until the action is complete
//
int spi_isf_complete() // wait until ISFlash command is completed 
{
  int bit,i,shiftval;
  int statuscmd = 0xD7;
  int isf_status;

  /* Flag Begin transmission first */
  spi_isf_csb_low();
 
  // status command
  spi_isf_send(statuscmd, 8);

 // continuosly obtain the status register until 'READY'
 // 8-bit status, bit7-0 
 // bit7 (MSBit):  0=busy, 1=ready 
  isf_status = spi_isf_send(0xFF, 8);
  // printf("status: 0x%x\n", (isf_status & 0x80));
  while ((isf_status & 0x80) == 0)
  {
     isf_status = spi_isf_send(0xFF, 8);
     // printf("ready: 0x%x\n", (isf_status & 0x80));
     // printf("Busy\n");
  }
  spi_isf_csb_high(); // end of transaction
  IPRINT1(1,"Ready: 0x%x \n",isf_status);
  // printf("Ready: 0x%x \n",isf_status);
  return isf_status & 0xFF;
}

/* ================================================================= */

/*
 * ICAT register based routines
 * These routines are LSBit first
 */

/* read data */
static unsigned xmit_spi_recv_bit(unsigned* value)
{
  unsigned data = *xmit_spi_rd;
  unsigned in   = icat_get_field(RF, xmit_spi_data_in, data);
  unsigned recv = (*value >> icat_get_field_width(RF,xmit_spi_data_in)) | (in<<((icat_spi_data_len-1)*icat_get_field_width(RF,xmit_spi_data_in)));
  return *value = recv;
}

static unsigned xmit_spi_send(unsigned value)
{
  unsigned in = 0; 
  unsigned in_tmp = 0; 
  int i; 

  /* LSB first */
  xmit_spi_select(ICAT_REG_SELECT);
  clk_out(0, 1, value&0x1);	/* clock low, frame high */
  xmit_spi_recv_bit(&in_tmp);	/* always read */
  for (i=0; i<icat_spi_word_len; i++) {
    clk_out(1, 1, value&0x1);	/* clock high, frame high */
    xmit_spi_recv_bit(&in_tmp); /* always read */
    value >>= 1;
    clk_out(0, 1, value&0x1);	/* clock low, frame high */
    xmit_spi_recv_bit(&in);	/* always read */
  }

  /* final pulse with frame low */
  clk_out(0, 0, 0);
  xmit_spi_release();
  /* mask off bits received when transmitting address and r/w */
  in &= (1<<icat_spi_data_len)-1;
  return in;
}

void icat_spi_write(unsigned addr, unsigned data)
{
  unsigned writeflag = 1;
  unsigned xmit;     /* word to be transmitted serially */
  xmit = icat_spi_word(addr, data, writeflag);
  if (debug) printf("xmit_spi_send(addr=0x%x wr=%x data=0x%x xmit=0x%x)\n", addr, writeflag, data, xmit);
  xmit_spi_send(xmit);
}

unsigned icat_spi_read(unsigned addr)
{
  unsigned writeflag = 0;
  unsigned dontcare = 0;	/* data portion of xmit */
  unsigned xmit;		/* word to be transmitted serially */
  unsigned data;		/* data portion of xmit */

  xmit = icat_spi_word(addr, dontcare, writeflag);
  data = xmit_spi_send(xmit);
  if (debug) printf("xmit_spi_send(addr=0x%x wr=%x data=0x%x xmit=0x%x)\n", addr, writeflag, data, xmit);
  return data;
}

unsigned dac_recv_bit(unsigned* value)
{
  unsigned data = *xmit_spi_rd;
  unsigned in   = icat_get_field(RF, xmit_spi_data_in, data);
  unsigned recv = (*value << icat_get_field_width(RF,xmit_spi_data_in)) |
    (in&((1<<icat_get_field_width(RF,xmit_spi_data_in))-1));
  if (debug > 5) printf("data = 0x%x",data);
  return *value = recv;
}

unsigned dac_spi_send(unsigned value)
{
  unsigned in = 0; 
  unsigned in_tmp = 0; 
  int i; 

  /* MSB first */
  xmit_spi_select(ICAT_DAC_SELECT);
  clk_out(0, 1, (value>>(ad9747_spi_word_len-1))&0x1);	/* clock low, frame high */
  dac_recv_bit(&in_tmp);
  for (i=0; i<ad9747_spi_word_len;) {
    clk_out(1, 1, (value>>((ad9747_spi_word_len-1)-i))&0x1);	/* clock high, frame high */
    dac_recv_bit(&in);
    i++;
    clk_out(0, 1, (value>>((ad9747_spi_word_len-1)-i))&0x1);	/* clock low, frame high */
    dac_recv_bit(&in_tmp);
  }

  /* final pulse with frame low */
  clk_out(0, 0, 0);
  xmit_spi_release();
  /* mask off bits received when transmitting address and r/w */
  in &= (1<<ad9747_spi_data_len)-1;
  return in;
}

static void set_base_addr(ficlVm *vm)
{
}

static FILE *logfile = NULL;

static void start_logging(ficlVm *vm)
{
  char buffer[256];
  char filename[256];
  ficlCountedString *counted = (ficlCountedString *)filename;

  ficlVmGetString(vm,counted,'\n');
  logfile = fopen(FICL_COUNTED_STRING_GET_POINTER(*counted),"w");
  if (logfile == NULL)
    {
      ficlVmTextOut(vm, "Unable to open file ");
      ficlVmTextOut(vm, FICL_COUNTED_STRING_GET_POINTER(*counted));
      ficlVmTextOut(vm, " for writing.\n");
      ficlVmThrow(vm, FICL_VM_STATUS_QUIT);
    }
}

static void stop_logging(ficlVm *vm)
{
  if (logfile != NULL)
    fclose(logfile);
  logfile = NULL;
}

static void spi_write(ficlVm *vm)
{
  ficlUnsigned32 addr;		/* address portion of xmit */
  ficlUnsigned32 data;		/* data portion of xmit */

  FICL_STACK_CHECK(vm->dataStack, 2, 0);
  addr = ficlStackPopInteger(vm->dataStack); /* register address offset */
  data = ficlStackPopInteger(vm->dataStack); /* data to write */

  if (logfile)
    {
      uint8_t logbuf[4];

      logbuf[0] = (addr>>8);
      logbuf[1] = addr;
      logbuf[2] = (data>>8);
      logbuf[3] = data;
      fwrite(logbuf,sizeof(logbuf),1,logfile);
    }

  icat_spi_write(addr, data);
}

static void spi_read(ficlVm *vm)
{
  ficlUnsigned32 addr;		/* address portion of xmit */
  ficlUnsigned32 data;		/* data read back */

  FICL_STACK_CHECK(vm->dataStack, 1, 1); /* only the address */
  addr = ficlStackPopInteger(vm->dataStack);
  data = icat_spi_read(addr);
  ficlStackPushInteger(vm->dataStack, data);
}

static void set_spi_debug(ficlVm *vm)
{
  ficlUnsigned32 d;

  FICL_STACK_CHECK(vm->dataStack, 1, 0);
  d = ficlStackPopInteger(vm->dataStack);
  debug = d;
}
  

static void dac_spi_write(ficlVm *vm)
{
  ficlUnsigned32 xmit;		/* word to be transmitted serially */
  ficlUnsigned32 addr;		/* address portion of xmit */
  ficlUnsigned32 data;		/* data portion of xmit */
  size_t i;

  FICL_STACK_CHECK(vm->dataStack, 2, 0);
  addr = ficlStackPopInteger(vm->dataStack); /* register address offset */
  data = ficlStackPopInteger(vm->dataStack); /* data to write */
  xmit = ad9747_spi_word(addr, data, 1);
  if (debug) printf("dac_spi_send(addr=0x%x wr=%x data=0x%x xmit=0x%x)\n", addr, 1, data, xmit);
  dac_spi_send(xmit);
}

static void dac_spi_read(ficlVm *vm)
{
  ficlUnsigned32 xmit;		/* word to be transmitted serially */
  ficlUnsigned32 addr;		/* address portion of xmit */
  ficlUnsigned32 dontcare = 0;	/* data portion of xmit */
  ficlUnsigned32 data;		/* data read back */

  FICL_STACK_CHECK(vm->dataStack, 1, 1); /* only the address */
  addr = ficlStackPopInteger(vm->dataStack);
  xmit = ad9747_spi_word(addr, dontcare, 0);
  data = dac_spi_send(xmit);
  if (debug) printf("dac_spi_send(addr=0x%x wr=%x data=0x%x xmit=0x%x) => 0x%x\n", addr, 0, data, xmit, data);
  ficlStackPushInteger(vm->dataStack, data);
}

static void ficl_spi_select(ficlVm *vm)
{
  /* Obsolete. Still here in case we have old FORTH code which makes this call */
}

static void ficl_flash_read(ficlVm *vm)
{
  ficlUnsigned32 one, two, three;

  FICL_STACK_CHECK(vm->dataStack, 3, 0);
  one = ficlStackPopInteger(vm->dataStack);
  two = ficlStackPopInteger(vm->dataStack);
  three = ficlStackPopInteger(vm->dataStack);
  readISF(one,two,three);
}

static void ficl_flash_write(ficlVm *vm)
{
  char buffer[256];
  char filename[256];
  ficlCountedString *counted = (ficlCountedString *)filename;
  FILE *fp;
  long fsize;
  char *buf;
  uint32_t addr;
  int status;

  addr = ficlStackPopInteger(vm->dataStack);

  ficlVmGetString(vm,counted,'\n');
  fp = fopen(FICL_COUNTED_STRING_GET_POINTER(*counted),"r");
  if (fp == NULL)
    {
      ficlVmTextOut(vm, "Unable to open file ");
      ficlVmTextOut(vm, FICL_COUNTED_STRING_GET_POINTER(*counted));
      ficlVmTextOut(vm, " for reading.\n");
      ficlVmThrow(vm, FICL_VM_STATUS_QUIT);
    }
  fseek(fp,0,SEEK_END);
  fsize = ftell(fp);
  printf("fsize: %d\n",fsize);
  buf = (char *)malloc(fsize);
  printf("buf: %x\n",buf);
  fseek(fp,0,SEEK_SET);
  status = fread(buf,fsize,1,fp);
  printf("fread: %d\n",status);
  fclose(fp);
  writeISF(addr,buf,fsize);
  printf("Wrote %d byte to address 0x%0x.\n",fsize,addr);
}

ficlUnsigned rf_spi_init(ficlVm *vm)
{
  ficlDictionary *dict;

  dict = ficlVmGetDictionary(vm);
  ficlDictionarySetPrimitive(dict,"start-logging",start_logging,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"stop-logging",stop_logging,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"spi-base!",set_base_addr,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"spi!",spi_write,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"spi@",spi_read,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"spi-debug!",set_spi_debug,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"spi-select",ficl_spi_select,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"dac!",dac_spi_write,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"dac@",dac_spi_read,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"flash!",ficl_flash_write,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"flash@",ficl_flash_read,FICL_WORD_DEFAULT);
  
  return 0;
}
