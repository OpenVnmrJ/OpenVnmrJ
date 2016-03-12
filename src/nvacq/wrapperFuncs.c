/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "vxWorks.h"
#include "stdio.h"
#include "envLib.h"
#include "usrLib.h"
#include "memLib.h"
#include "excLib.h"
#include "logLib.h"
#include "vxLib.h"
#include "string.h"
#include "taskLib.h"

#include "memDrv.h"  /* scripts & ld */
#include "loadLib.h"
#include "shellLib.h"
#include "time.h"
#include "logMsgLib.h"

/*
*  to replace a library routine with your own you can use the wrapper
* 1) Use the linker option "--wrap symbol", you should be
*   able to link your ld() API without modifying the VxWorks
*   library.
*    Use a wrapper function for symbol. Any undefined reference to symbol will 
*    be resolved to __wrap_symbol. Any undefined reference to __real_symbol will 
*    be resolved to symbol.
*
*   This can be used to provide a wrapper for a system function. The wrapper 
*   function should be called __wrap_symbol. If it wishes to call the system 
*   function, it should call __real_symbol.
*
*    Here is a trivial example:
*
* void *
* __wrap_malloc (int c)
* {
*   printf ("malloc called with %ld\n", c);
*   return __real_malloc (c);
* }
* 
*    If you link other code with this file using --wrap malloc, then all calls 
*    to malloc will call the function __wrap_malloc instead. The call to 
*    __real_malloc in __wrap_malloc will call the real malloc function.
*
* You may wish to provide a __real_malloc function as well, so that links 
* without the --wrap option will succeed. If you do this, you should not 
* put the definition of __real_malloc in the same file as __wrap_malloc; 
* if you do, the assembler may resolve the call before the linker has a 
* chance to wrap it to malloc. 
*
*
* in '/sw/windT2_2_PPC/target/h/make' file rules.bsp 
* 
* vxWorks.st : depend.$(BSP_NAME) usrConfig_st.o dataSegPad.o \
*        $(LD) $(LDFLAGS) $(LD_ENTRY_OPT) $(SYS_ENTRY) $(LD_LOW_FLAGS) \
*            -o $@ --wrap ld tmp.2 symTbl.o ctdt.o $(LD_SCRIPT_RAM)
*
*/
/*
*  diagnostic memory allocation wrappers
*
* in '/sw/windT2_2_PPC/target/h/make' file rules.bsp 
* 
* vxWorks.st : depend.$(BSP_NAME) usrConfig_st.o dataSegPad.o \
*        $(LD) $(LDFLAGS) $(LD_ENTRY_OPT) $(SYS_ENTRY) $(LD_LOW_FLAGS) \
*            -o $@  --wrap memPartAlloc --wrap memPartAlignedAlloc --wrap valloc --wrap realloc --wrap malloc --wrap free --wrap calloc --wrap memalign tmp.2 symTbl.o ctdt.o $(LD_SCRIPT_RAM)
*
*
* #define MALLOC_WRAPPER
*/

// #define MALLOC_WRAPPER
#ifdef MALLOC_WRAPPER

typedef struct {
   void *pointer;
   int   taskID;
   int   PC;
   int   size;
} MALLOC_ENTRY;

// typedef MALLOC_ENTRY *MALLOC_ID;

//typedef struct {
//   int   num_entries;
//   MALLOC_ENTRY  mallocEntried[4096];
//   int   max_entries;
//} MALLOC_OBJ;

static int num_entries = 0;
static int hiWaterMark = 0;
static MALLOC_ENTRY  mallocEntried[4096];
static int max_entries = 4096;

// typedef MALLOC_OBJ *MOBJ_ID;


static int mallocSuspendFlag = 0;  /* if one then suspend task match the custom test */
static int suspendTaskId = 0;      /* suspend task ID saved, so it my be resumed */
static int mallocOutput = 0;   /* if 1 then print diagnostic output */

void * __wrap_calloc (int n, int c)  {
   void *pointer;
   int taskID,PC;
   pointer = __real_calloc (n,c);

   if (mallocOutput == 0)
      return(pointer);

   taskID = taskIdSelf();  /* this task ID of the calling function */
   PC = pc(taskID);
   printf("'%s': calloc called with %ld elements,%ld size, return Addr: 0x%lx, PC=0x%lx\n",
   taskName(taskID), n,c, pointer,PC);
   if ( (c == 48) )
   {
      ti(taskID);
      if (mallocSuspendFlag == 1)
      {
         suspendTaskId = taskID;
         printf("'%s': Suspending calloc task\n", taskName(taskID));
         taskSuspend(taskID);
      }
      /* tt(taskID); */ /* stack trace of calling function/task */
   }
   return pointer;
}
void * __wrap_realloc (void* blk, int c)  {
   void *pointer;
   int taskID,PC;
   pointer = __real_realloc (blk,c);

   if (mallocOutput == 0)
      return(pointer);

   taskID = taskIdSelf();  /* this task ID of the calling function */
   PC = pc(taskID);
   printf("'%s': realloc called with 0x%lx block, %ld size, return Addr: 0x%lx, PC=0x%lx\n",
   taskName(taskID), blk,c, pointer,PC);
   if ( (c == 48) )
   {
      ti(taskID);
      if (mallocSuspendFlag == 1)
      {
         suspendTaskId = taskID;
         printf("'%s': Suspending calloc task\n", taskName(taskID));
         taskSuspend(taskID);
      }
      /* tt(taskID); */ /* stack trace of calling function/task */
   }
   return pointer;
}


void * __wrap_memalign (int n, int c)  {
   void *pointer;
   int taskID,PC;
   pointer = __real_memalign (n,c);

   if (mallocOutput == 0)
      return(pointer);

   taskID = taskIdSelf();  /* this task ID of the calling function */
   PC = pc(taskID);
   printf("'%s': calloc called with %ld aligned, %ld size, return Addr: 0x%lx, PC=0x%lx\n",
   taskName(taskID), n,c, pointer,PC);
   if ( (c == 48) )
   {
      ti(taskID);
      if (mallocSuspendFlag == 1)
      {
         suspendTaskId = taskID;
         printf("'%s': Suspending malloc task\n", taskName(taskID));
         taskSuspend(taskID);
      }
      /* tt(taskID); */ /* stack trace of calling function/task */
   }
   return pointer;
}
void * __wrap_memPartAlloc(void* partId, unsigned int bytes) {
   void *pointer;
   int taskID,PC;
   pointer = __real_memPartAlloc (partId, bytes);

   if (mallocOutput == 0)
      return(pointer);

   taskID = taskIdSelf();  /* this task ID of the calling function */
   PC = pc(taskID);
   printf("'%s': memPartAlloc called with %ld, return Addr: 0x%lx, PC=0x%lx\n",
      taskName(taskID), bytes, pointer,PC);
   if ( (bytes == 48) )
   {
      ti(taskID);
      if (mallocSuspendFlag == 1)
      {
         suspendTaskId = taskID;
         printf("'%s': Suspending memPartAlloc task\n", taskName(taskID));
         taskSuspend(taskID);
      }
      /* tt(taskID); */ /* stack trace of calling function/task */
      printf("'%s': memPartAlloc called with %ld, return Addr: 0x%lx, PC=0x%lx\n", taskName(taskID), bytes, pointer,PC);
   }
   return pointer;
}

void * __wrap_memPartAlignedAlloc(void* partId, unsigned int bytes,int align) {
   void *pointer;
   int taskID,PC;
   pointer = __real_memPartAlignedAlloc (partId, bytes, align);

   if (mallocOutput == 0)
      return(pointer);

   taskID = taskIdSelf();  /* this task ID of the calling function */
   PC = pc(taskID);
   printf("'%s': memPartAlignAlloc called with %ld,align: %ld return Addr: 0x%lx, PC=0x%lx\n",
      taskName(taskID), bytes, align, pointer,PC);
   if ( (bytes == 48) )
   {
      ti(taskID);
      if (mallocSuspendFlag == 1)
      {
         suspendTaskId = taskID;
         printf("'%s': Suspending memPartAlloc task\n", taskName(taskID));
         taskSuspend(taskID);
      }
      /* tt(taskID); */ /* stack trace of calling function/task */
   }
   return pointer;
}
void * __wrap_valloc (int c)  {
   void *pointer;
   int taskID,PC;
   pointer = __real_valloc (c);

   if (mallocOutput == 0)
      return(pointer);

   taskID = taskIdSelf();  /* this task ID of the calling function */
   PC = pc(taskID);
   printf("'%s': valloc called with %ld, return Addr: 0x%lx, PC=0x%lx\n", taskName(taskID), c, pointer,PC);
   if ( (c == 48) )
   {
      ti(taskID);
      if (mallocSuspendFlag == 1)
      {
         suspendTaskId = taskID;
         printf("'%s': Suspending valloc task\n", taskName(taskID));
         taskSuspend(taskID);
      }
      /* tt(taskID); */ /* stack trace of calling function/task */
   }
   return pointer;
}
/* diagnostic malloc */
void * __wrap_malloc (int c)  {
   void *pointer;
   int taskID,PC;
   taskID = taskIdSelf();  /* this task ID of the calling function */
   PC = pc(taskID);
   pointer = __real_malloc (c);
   
   if (num_entries < 4096) {
     mallocEntried[num_entries].pointer = pointer;
     mallocEntried[num_entries].taskID = taskID;
     mallocEntried[num_entries].PC = PC;
     mallocEntried[num_entries].size = c;
     num_entries++;
     hiWaterMark = (num_entries > hiWaterMark ) ? num_entries : hiWaterMark;
   }
   else
   {
     DPRINT1(-9,"'%s': malloc max entries for debug reached, further malloc not registered.\n", taskName(taskID));
   }

   if (mallocOutput == 0)
      return(pointer);

  //  taskID = taskIdSelf();  /* this task ID of the calling function */
   // PC = pc(taskID);
   // printf("'%s': malloc called with %ld, return Addr: 0x%lx, PC=0x%lx\n", taskName(taskID), c, pointer,PC);
   DPRINT4(-9,"'%s': malloc called with %ld, return Addr: 0x%lx, PC=0x%lx\n", taskName(taskID), c, pointer,PC);
   if ( (c == 48) )
   {
      ti(taskID);
      if (mallocSuspendFlag == 1)
      {
         suspendTaskId = taskID;
         // printf("'%s': Suspending malloc task\n", taskName(taskID));
         DPRINT1(-9,"'%s': Suspending malloc task\n", taskName(taskID));
         taskSuspend(taskID);
      }
      /* tt(taskID); */ /* stack trace of calling function/task */
      // printf("'%s': malloc called with %ld, return Addr: 0x%lx, PC=0x%lx\n", taskName(taskID), c, pointer,PC);
      DPRINT4(-9,"'%s': malloc called with %ld, return Addr: 0x%lx, PC=0x%lx\n", taskName(taskID), c, pointer,PC);
   }
   return pointer;
}

removeEntry(void *ptr)
{
   int i;
      for (i=0; i < max_entries; i++)
      {
           if (mallocEntried[i].pointer == ptr)
           {
               DPRINT4(-9,"removeEntry: Remove Malloc Task: '%s', Malloc Addr: 0x%lx, bytes: %d, PC:  0x%lx\n", 
                  taskName(mallocEntried[i].taskID),
                  mallocEntried[i].pointer, mallocEntried[i].size, mallocEntried[i].PC);
               mallocEntried[i].pointer = NULL;
               mallocEntried[i].taskID = NULL;
               mallocEntried[i].PC = NULL;
               mallocEntried[i].size = 0;
               num_entries--;
           }
      }
}

int __wrap_free (int addr)  {
   int status;
   int taskID,PC;
   status = __real_free (addr);
   removeEntry((void*)addr);
   if (mallocOutput == 0)
      return(status);
   taskID = taskIdSelf();  /* this task ID of the calling function */
   PC = pc(taskID);
   // printf("'%s': free called with Addr: 0x%lx, PC = 0x%lx\n", taskName(taskID), addr,PC);
   DPRINT3(-9,"'%s': free called with Addr: 0x%lx, PC = 0x%lx\n", taskName(taskID), addr,PC);
   /* ti(taskID); */
   /* tt(taskID); */ /* stack trace of calling function/task */
   /* printf("'%s': free called with Addr: 0x%lx, PC = 0x%lx\n", taskName(taskID), addr,PC); */
   return status;
}

/* clear output flag, suspress output */
kmallocOutputClr()
{
   mallocOutput = 0;
}

/* enable output from warpper memory allocation routines */
kmallocOutput()
{
   mallocOutput = 1;
}

/* suspend task match the custom critiria */
kSuspendMallocTask()
{
   mallocSuspendFlag = 1;
   return 0;
}

/* resume task that was suspended */
kResumeMallocTask()
{
   mallocSuspendFlag = 0;
   taskResume(suspendTaskId);
   return 0;
}

initMallocDebug()
{
  int i;
  
  num_entries = 0;
  hiWaterMark = 0;
  for (i=0; i < max_entries; i++)
  {
      mallocEntried[i].pointer = NULL;
      mallocEntried[i].taskID = NULL;
      mallocEntried[i].PC = NULL;
      mallocEntried[i].size = 0;
  }
}

mallocShow()
{
   int i;
   int count;
   printf("Entries: %d\n",num_entries);
   printf("High Water Mark: %d\n",hiWaterMark);
   if ( num_entries > 0 )
   {
      printf("Mallocs not Freed\n");
      count = 1;
      for (i=0; i < max_entries; i++)
      {
           if (mallocEntried[i].pointer != NULL)
           {
               printf("%d - Malloc Task: '%s', Malloc Addr: 0x%lx, bytes: %d, PC:  0x%lx\n", 
                  count,taskName(mallocEntried[i].taskID),
                  mallocEntried[i].pointer, mallocEntried[i].size, mallocEntried[i].PC);
              count++;
           }
      }
  }
}

#endif 
