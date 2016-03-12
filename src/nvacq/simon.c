/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
** stub main for testing Ficl
** Id: //depot/gamejones/ficl/main.c#2
*/
/*
** Copyright (c) 1997-2001 John Sadler (john_sadler@alum.mit.edu)
** All rights reserved.
**
** Get the latest Ficl release at http://ficl.sourceforge.net
**
** I am interested in hearing from anyone who uses Ficl. If you have
** a problem, a success story, a defect, an enhancement request, or
** if you would like to contribute to the Ficl release, please
** contact me by email at the address above.
**
** L I C E N S E  and  D I S C L A I M E R
** 
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <semLib.h>
#include "ledLib.h"

#include <sysSymTbl.h>

#include "ficl.h"
#include "rf.h"
#include "fpgaBaseISR.h"
#include "simon.h"

#include "vTypes.h"
#include "vWareLib.h"
#include "icatISF.h"
#include "sysSymTbl.h"
#include "taskLib.h"
#include "logMsgLib.h"

extern int DebugLevel;

ficlSystem *simon_system = NULL;

static SEM_ID finished_sem = NULL;

static int fifo_status = 0;

static void handle_fifo_finished(int param)
{
  fifo_status = param;
  semGive(finished_sem);
}

static void install_fifo_finished(void)
{
  if (!finished_sem)
    {
      finished_sem = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
      fpgaIntConnect(handle_fifo_finished,0,1<<RF_fifo_finished_status_pos);
      fpgaIntConnect(handle_fifo_finished,1,1<<RF_fifo_underflow_status_pos);
      fpgaIntConnect(handle_fifo_finished,2,1<<RF_fifo_overflow_status_pos);
    }
}
  
static void ficl_wait_fifo(ficlVm *vm)
{
  semTake(finished_sem,WAIT_FOREVER);
}

static void ficl_fifo_return_status(ficlVm *vm)
{
  FICL_STACK_CHECK(vm->dataStack,0,1);
  ficlStackPushInteger(vm->dataStack,fifo_status);
}

static void ficl_task_delay(ficlVm *vm)
{
  int delay;
  FICL_STACK_CHECK(vm->dataStack,1,0);
  delay = ficlStackPopInteger(vm->dataStack);
  taskDelay(delay);
}

extern void rf_spi_init(ficlVm *);

extern void sysDcrEbcSet(uint32_t, uint32_t);
extern uint32_t sysDcrEbcGet(uint32_t);
extern uint32_t sysDcrCr0Get(void);
extern void sysDcrCr0Set(uint32_t);

static void ficl_sysDcrEbcSet(ficlVm *vm)
{
  uint32_t addr, val;
  FICL_STACK_CHECK(vm->dataStack,2,0);
  addr = ficlStackPopInteger(vm->dataStack);
  val = ficlStackPopInteger(vm->dataStack);
  sysDcrEbcSet(addr,val);
}

static void ficl_sysDcrEbcGet(ficlVm *vm)
{
  uint32_t addr, val;
  FICL_STACK_CHECK(vm->dataStack,1,1);
  addr = ficlStackPopInteger(vm->dataStack);
  val = sysDcrEbcGet(addr);
  ficlStackPushInteger(vm->dataStack,val);
}

static void ficl_sysDcrCr0Get(ficlVm *vm)
{
  uint32_t val;
  FICL_STACK_CHECK(vm->dataStack,0,1);
  val = sysDcrCr0Get();
  ficlStackPushInteger(vm->dataStack,val);
}

static void ficl_sysDcrCr0Set(ficlVm *vm)
{
   uint32_t val;
   FICL_STACK_CHECK(vm->dataStack, 1, 0);
   val = ficlStackPopInteger(vm->dataStack);
   sysDcrCr0Set(val);
}
 
static void ficl_ffdir(ficlVm * vm)
{
    ffdir("*");
}

#define ICAT_DEV_PREFIX "icat:"
#define FLASH_DEV_PREFIX "ffs:"
#define NFS_DEV_PREFIX "rsh:"

char *find_and_load(const char *path, unsigned *size, int *dataOffset)
{
  char *buffer;
  DPRINT1(3,"find_and_load(%s)\n", path);
  *dataOffset = 0;
  if (strncmp(ICAT_DEV_PREFIX,path,sizeof(ICAT_DEV_PREFIX)-1) == 0)
    {
      const char *filename = path+sizeof(ICAT_DEV_PREFIX)-1;
      buffer = readIsfFile(filename,size,dataOffset);
      DPRINT4(3,"readIsfFile(%s) returned %d bytes offset %d @0x%x\n", filename, *size, *dataOffset, buffer);
    }
  else if (strncmp(FLASH_DEV_PREFIX,path,sizeof(FLASH_DEV_PREFIX)-1) == 0)
    {
      FFSHANDLE fileHdl;
      unsigned cnt;
      const char *filename = path+sizeof(FLASH_DEV_PREFIX)-1;
      
      if ((fileHdl = vfOpenFile((char *)filename)) == NULL)
	{
	  *size = 0;
	  return NULL;
	}

      *size = vfFilesize(fileHdl);
      if ((buffer = (char *) malloc(*size)) == NULL)
	{
	  vfCloseFile(fileHdl);
	  *size = 0;
	  return NULL;
	}

      vfReadFile(fileHdl,0,buffer,*size,&cnt);
      if (cnt != *size)
	{
	  free(buffer);
	  vfCloseFile(fileHdl);
	  *size = 0;
	  return 0;
	}

      vfCloseFile(fileHdl);
    }
  else
    {
      FILE *fp;
      const char* filename;
      if (strncmp(NFS_DEV_PREFIX,path,sizeof(NFS_DEV_PREFIX)-1) == 0)
	filename = path+sizeof(NFS_DEV_PREFIX)-1;
      else
	filename = path;

      if ((fp = fopen(filename,"r")) == NULL)
	{
	  *size = 0;
	  return NULL;
	}

      fseek(fp,0,SEEK_END);
      *size = ftell(fp);
      fseek(fp,0,SEEK_SET);
      if ((buffer = malloc(*size+1)) == NULL)
	{
	  *size = 0;
	  fclose(fp);
	  return NULL;
	}
      if (fread(buffer,1,*size,fp) < *size)
	{
	  *size = 0;
	  free(buffer);
	  fclose(fp);
	  return NULL;
	}
      buffer[*size] = '\0';
      fclose(fp);
    }
  return buffer;
}

static void ficlPrimitiveLoad_(ficlVm *vm, int warn, int icat_special)
{
    char filename[FICL_COUNTED_STRING_MAX];
    ficlCountedString *counted = (ficlCountedString *)filename;
    char *file_data;
    unsigned size = 0;
    int line_num;
    char *line, *c, *end;
    ficlCell oldSourceId;
    ficlString s;
    int result = 0;
    char *context;
    int offset;

    if (icat_special)
      {
	int index;
	FICL_STACK_CHECK(vm->dataStack,1,0);
	index = ficlStackPopInteger(vm->dataStack);
	sprintf(filename,"%sicat_config%02d.tbl",ICAT_DEV_PREFIX,index);
	file_data = find_and_load(filename,&size,&offset);
      }
    else
      {
	ficlVmGetString(vm, counted, '\n');

	if (FICL_COUNTED_STRING_GET_LENGTH(*counted) <= 0)
	  {
	    ficlVmTextOut(vm, "Warning (load): nothing happened\n");
	    return;
	  }
	file_data = find_and_load(FICL_COUNTED_STRING_GET_POINTER(*counted),&size,&offset);
      }

    /*
    ** get the file's size and make sure it exists 
    */

    if (file_data == NULL)
      if (warn)
	{
	  ficlVmTextOut(vm, "Unable to open file ");
	  ficlVmTextOut(vm, FICL_COUNTED_STRING_GET_POINTER(*counted));
	  ficlVmTextOut(vm, "\n");
	  ficlVmThrow(vm, FICL_VM_STATUS_QUIT);
	}
      else
	return;

    oldSourceId = vm->sourceId;
    vm->sourceId.i = -1;

    /* feed each line to ficlExec */
    for (end = file_data + size + offset, line = file_data + offset, line_num = 1; line < end; line = ++c, line_num++)
      {
	int length;
	for (c = line; *c != '\n' && c<end; c++);
	length = c-line;
	if (length > 0)
	  {
	    FICL_STRING_SET_POINTER(s, line);
	    FICL_STRING_SET_LENGTH(s, length);
	    result = ficlVmExecuteString(vm, s);
	    /* handle "bye" in loaded files. --lch */
	    switch (result)
	      {
	      case FICL_VM_STATUS_OUT_OF_TEXT:
	      case FICL_VM_STATUS_USER_EXIT:
                break;

	      default:
                vm->sourceId = oldSourceId;
		free(file_data);
		file_data = NULL;
                ficlVmThrowError(vm, "Error loading file <%s> line %d", FICL_COUNTED_STRING_GET_POINTER(*counted), line_num);
                break; 
	      }
	  }
      }

    if (file_data != NULL) free(file_data);

    vm->sourceId = oldSourceId;

    /* handle "bye" in loaded files. --lch */
    if (result == FICL_VM_STATUS_USER_EXIT)
        ficlVmThrow(vm, FICL_VM_STATUS_USER_EXIT);
    return;
}

static void ficlPrimitiveLoad(ficlVm *vm)
{
  ficlPrimitiveLoad_(vm,1,0);
}

static void ficlPrimitiveLoadQuestion(ficlVm *vm)
{
  ficlPrimitiveLoad_(vm,0,0);
}

static void ficlIcatLoad(ficlVm *vm)
{
  ficlPrimitiveLoad_(vm,0,1);
}

static void ficlIcatFileWrite(ficlVm *vm)
{
  uint32_t len;
  char *buffer, md5[36];
  char filename[FICL_COUNTED_STRING_MAX];
  ficlCountedString *counted = (ficlCountedString *)filename;

  FICL_STACK_CHECK(vm->dataStack,2,0);
  len = ficlStackPopUnsigned(vm->dataStack);
  buffer = (char *) ficlStackPopPointer(vm->dataStack);
  calcMd5(buffer,len,md5);
  ficlVmGetString(vm, counted, '\n');
  writeIsfFile(FICL_COUNTED_STRING_GET_POINTER(*counted),md5,len,buffer,1);
}

static void ficlIcatPFileWrite(ficlVm *vm)
{
  uint32_t len;
  char *buffer, md5[36];
  uint32_t filename_len;
  char *filename;

  FICL_STACK_CHECK(vm->dataStack,2,0);
  filename_len = ficlStackPopUnsigned(vm->dataStack);
  filename = (char *) ficlStackPopPointer(vm->dataStack);
  len = ficlStackPopUnsigned(vm->dataStack);
  buffer = (char *) ficlStackPopPointer(vm->dataStack);
  calcMd5(buffer,len,md5);
  writeIsfFile(filename,md5,len,buffer,1);
}

static void ficlIsfDir(ficlVm *vm)
{
  isfdir();
}

static void ficlIsfDel(ficlVm *vm)
{
  char filename[FICL_COUNTED_STRING_MAX];
  ficlCountedString *counted = (ficlCountedString *)filename;

  ficlVmGetString(vm, counted, '\n');
  isfdel(FICL_COUNTED_STRING_GET_POINTER(*counted));
}

static void ficlPcp(ficlVm *vm)
{
  char srcfile[FICL_COUNTED_STRING_MAX];
  ficlCountedString *srccounted = (ficlCountedString *)srcfile;
  char dstfile[FICL_COUNTED_STRING_MAX];
  ficlCountedString *dstcounted = (ficlCountedString *)dstfile;

  ficlVmGetString(vm, srccounted, ' ');
  ficlVmGetString(vm, dstcounted, '\n');

  pcp(FICL_COUNTED_STRING_GET_POINTER(*srccounted),FICL_COUNTED_STRING_GET_POINTER(*dstcounted));
}

static void ficl_reboot(ficlVm *vm)
{
  void reboot(int);

  reboot(0);
}

void ficl_diagPrint(ficlVm *vm)
{
  int len;
  char *buffer;

  FICL_STACK_CHECK(vm->dataStack,2,0);
  len = ficlStackPopUnsigned(vm->dataStack);
  buffer = (char *) ficlStackPopPointer(vm->dataStack);
  diagPrint(" ",buffer);
}

static int spawn_helper
(
 int arg0,
 int arg1,
 int arg2,
 int arg3,
 int arg4,
 int arg5,
 int arg6,
 int arg7,
 int arg8,
 int arg9
 )
{
  unsigned command = arg0;
  char *taskName = (char *)arg1;
  ficlVm *vm;
  char eval[20];
  sprintf(eval,"0x%x execute\n",command);
  
  vm = ficlSystemCreateVm(simon_system);
  ficlVmEvaluate(vm,eval);
  ficlSystemDestroyVm(vm);
  free(taskName);
  return 0;
}

void ficl_taskSpawn(ficlVm *vm)
{
  int tNameLen;
  char *tName;
  int priority;
  int flags;
  int stackSize;
  unsigned command;

  char *taskName;

  FICL_STACK_CHECK(vm->dataStack,6,0);

  tNameLen = ficlStackPopInteger(vm->dataStack);
  tName =  ficlStackPopPointer(vm->dataStack);
  priority = ficlStackPopInteger(vm->dataStack);
  flags = ficlStackPopInteger(vm->dataStack);
  stackSize = ficlStackPopInteger(vm->dataStack);
  command = ficlStackPopUnsigned(vm->dataStack);

  if (simon_system == NULL)
    simon_boot(NULL);

  taskName = malloc(strlen(tName)+1);
  strcpy(taskName,tName);

  taskSpawn(taskName,priority,flags,stackSize,
	    spawn_helper,command,(int)taskName,0,0,0,0,0,0,0,0);
}

extern int dds2_ftw1(double freq);
extern int dds2_ftw2(double freq);
extern int dds2_switch(double freq);

static void ficl_ftw1(ficlVm *vm)
{
  FICL_STACK_CHECK(vm->dataStack,0,1);
  FICL_STACK_CHECK(vm->floatStack,1,0);
  ficlStackPushUnsigned(vm->dataStack,dds2_ftw1(ficlStackPopFloat(vm->floatStack)));
}

static void ficl_ftw2(ficlVm *vm)
{
  FICL_STACK_CHECK(vm->dataStack,0,1);
  FICL_STACK_CHECK(vm->floatStack,1,0);
  ficlStackPushUnsigned(vm->dataStack,dds2_ftw2(ficlStackPopFloat(vm->floatStack)));
}

static void ficl_switch(ficlVm *vm)
{
  FICL_STACK_CHECK(vm->dataStack,0,1);
  FICL_STACK_CHECK(vm->floatStack,1,0);
  ficlStackPushUnsigned(vm->dataStack,dds2_switch(ficlStackPopFloat(vm->floatStack)));
}

void simon_init(ficlVm *vm)
{
  ficlDictionary *dict;
  dict = ficlVmGetDictionary(vm);
  ficlDictionarySetPrimitive(dict,"dcr!",ficl_sysDcrEbcSet,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"dcr@",ficl_sysDcrEbcGet,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"load",ficlPrimitiveLoad,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"load?",ficlPrimitiveLoadQuestion,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"file!",ficlIcatFileWrite,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"pfile!",ficlIcatPFileWrite,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"isfdir",ficlIsfDir,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"isfdel",ficlIsfDel,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"pcp",ficlPcp,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"icat-load",ficlIcatLoad,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"task-delay",ficl_task_delay,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"wait-fifo",ficl_wait_fifo,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"fifo-return-status",ficl_fifo_return_status,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"reboot",ficl_reboot,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"task-spawn",ficl_taskSpawn,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"diag-print",ficl_diagPrint,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"ftw1",ficl_ftw1,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"ftw2",ficl_ftw2,FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(dict,"rfswitch",ficl_switch,FICL_WORD_DEFAULT);
  install_fifo_finished();
}

int simon_boot(char *eval)
{
  ficlVm *vm;
  ficlSystemInformation fsi;

  ficlSystemInformationInitialize(&fsi);
  fsi.dictionarySize = 256*1024;

  simon_system = ficlSystemCreate(&fsi);
  ficlSystemCompileExtras(simon_system);
  vm = ficlSystemCreateVm(simon_system);
  simon_init(vm);
  rf_spi_init(vm);

  if (eval != NULL) ficlVmEvaluate(vm,eval);
  ficlSystemDestroyVm(vm);
  return 0; 
}

ficlSystem *get_simon_system ()
{
   return simon_system;
}

void simon(void)
{
  int returnValue = 0;
  char buffer[256];
  ficlVm *vm;
  int led_id;

  if (simon_system == NULL)
    simon_boot(NULL);

  vm = ficlSystemCreateVm(simon_system);

  led_id = ledOpen(fileno(stdin),fileno(stdout),100);

  while (returnValue != FICL_VM_STATUS_USER_EXIT)
    {
      fputs(FICL_PROMPT, stdout);
      fflush(stdout);
      ledRead(led_id,buffer,sizeof(buffer));
      returnValue = ficlVmEvaluate(vm, buffer);
    }

  ledClose(led_id);

  ficlSystemDestroyVm(vm);
}

void full_simon(void)
{
  int returnValue = 0;
  char buffer[256];
  ficlVm *vm;
  ficlSystem *system;
  ficlSystemInformation fsi;
  ficlDictionary *dict;
  FILE *fd;
  int led_id;

  ficlSystemInformationInitialize(&fsi);
  fsi.dictionarySize = 256*1024;

  printf("creating ficl system\n");
  system = ficlSystemCreate(&fsi);

  printf("compiling ficl extras\n");
  ficlSystemCompileExtras(system);

  printf("creating ficl vm\n");
  vm = ficlSystemCreateVm(system);

  printf("initializing silkworm customizations\n");
  rf_spi_init(vm);

  printf("getting ficl version\n");
  returnValue = ficlVmEvaluate(vm, ".ver .( " __DATE__ " ) cr quit");

  simon_init(vm);
  rf_spi_init(vm);

  printf("ficl initialization complete\n");

  led_id = ledOpen(fileno(stdin),fileno(stdout),100);

  while (returnValue != FICL_VM_STATUS_USER_EXIT)
    {
      fputs(FICL_PROMPT, stdout);
      fflush(stdout);
      ledRead(led_id,buffer,sizeof(buffer));
      returnValue = ficlVmEvaluate(vm, buffer);
    }

  ledClose(led_id);

  ficlSystemDestroy(system);
}
