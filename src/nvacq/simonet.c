/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#ifndef SIMONET_TEST
#  include <symLib.h>
#  include <sysSymTbl.h>
#  include "taskLib.h"
#  include "taskPriority.h"
#endif
#include "logMsgLib.h"
#include "ficl.h"
#include "simonet.h"

#define MAXMSG 1024             /* maximum message size from remote ficl server client */

/*
 * simonet usage:
 *   starting the simonet server:   simonet portNumber, StackSize
 *       e.g.  -> simonet 555,15000
 *
 * Syntax to simonet: string with '\n' at end.
 *    Remember forth is reverse polish notation oriented
 *  e.g.   "2 1 + .\n"   pure forth,  2 + 1 print answer  3
 *
 *  Call a 'C' function with integer args:  'arg2 arg1 nargs s" C function name" nv \n' 
 *   C function must a string denoted via 's" ' syntax, space after 's"' is Required!
 *
 *  e.g.  '0 s" prtnddsver" nv\n'
 *
 *  Call a 'C' function with float args:  'arg2 arg1 nargs s" C function name" nvf \n' 
 */
extern ficlSystem *get_simon_system();

void add_simonet_primitives(ficlSystem*);

// forth virtual machine initialization hook
void (*simon_vm_init_hook)(ficlVm * vm) = NULL;

int alloc_socket(int port)
{
   struct sockaddr_in sockaddr;

   /* Create the socket. */
   int sock = socket(PF_INET, SOCK_STREAM, 0);
   if (sock < 0)
   {
      perror("socket");
      exit(EXIT_FAILURE);
   }

   /* Give the socket a name. */
   bzero((void *) &sockaddr, sizeof(sockaddr));
   sockaddr.sin_family = AF_INET;
   sockaddr.sin_port = htons(port);
   sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
   if (bind(sock, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) < 0)
   {
      perror("bind");
      exit(EXIT_FAILURE);
   }

   return sock;
}

/**
 * Communication occurs via reads and writes to file descriptors.
 * Ficl responses occur via simonCallbackTextOut and the file descriptor
 * associated with each VM, which is registered with each VM.
 */
#define EOT '\4'  // ^D or EOF character
int process_client_request(int filedes, ficlVm * vm)
{
   char buffer[MAXMSG];
   int nbytes = read(filedes, buffer, MAXMSG);
   
   if (nbytes < 0)              /* Read error. */
   {
      perror("read");
      return -1;
   }
   else if ((nbytes == 0) || (*buffer == EOT))        /* End-of-File. */
   {
      return -1;
   }
   else                         /* Data read. */
   {
      buffer[nbytes] = 0;
      fprintf(stderr, "Server[0x%x]: got %d byte message: \"%s\"\n", (unsigned) vm, nbytes, buffer);
      buffer[nbytes - 1] = '\n';        /*necessary? */
      ficlVmEvaluate(vm, buffer);
      return 1;
   }
}

void* simon_get_datastack(ficlVm* vm)
{
  return vm->dataStack;
}

void simonCallbackTextOut(ficlCallback * callback, char *message)
{
   int filedes = (int) callback->context;
   int nbytes = 0;
   int msgsize = 0;
   static char eoln = '\n';
   char sizestr[33];

   if (message != NULL)
   {
      nbytes = write(filedes, message, strlen(message));
      //write (filedes, &eoln, 1);
      if (nbytes < 0) {
         perror("simonCallbackTextOut write");
	 if (errno == EPIPE) {
	   printf("remote client closed %d\n", filedes);
	 }
      }
   }
   else
   {     
       /* no fsync!? Do nothing for now...
          nbytes = fsync(filedes);
          if (nbytes < 0)
          perror("simonCallbackTextOut fsync");
       */
   }
}

void simon_fsi_init(ficlSystemInformation *fsi)
{
   ficlSystemInformationInitialize(fsi);
}

void simonize(ficlVm * vm, const char* cmd)
{
   ficlVm* vm_ptr = vm;
   if (vm == NULL)
     vm_ptr = simon_vm_alloc();
   ficlVmEvaluate(vm_ptr, (char *) cmd);
   if (vm == NULL)
     simon_vm_destroy(vm_ptr);
}

void simon_vm_destroy(ficlVm * vm)
{
   ficlSystemDestroyVm(vm);
}

void simon_vm_init(ficlVm * vm, int filedes)
{
   extern void simon_init(ficlVm *);
   // allow an alternative file descriptor for sockets
   if (filedes >= 0) {
     FICL_VM_ASSERT(vm, sizeof(vm->callback.context) >= sizeof(filedes));
     ficlVmSetTextOut(vm, simonCallbackTextOut);
     vm->callback.errorOut = simonCallbackTextOut; // send errors back to socket as well
     vm->callback.context = (void *) filedes;
   }

   simon_init(vm);

   if (simon_vm_init_hook != NULL)
     simon_vm_init_hook(vm);
}

void simon_hook_vm_init(void (*hook)(ficlVm * vm))
{
  if (simon_vm_init_hook != NULL)
    simon_vm_init_hook = hook;
  else
    DPRINT(-2,"ERROR: A simon virtual machine initializer has already been registered!");
}

ficlSystem* simonet_boot()
{
   ficlSystem* simon_system = get_simon_system();
   if (simon_system == NULL) {
     DPRINT(0, "booting simon");
     simon_boot(NULL);
     simon_system = get_simon_system();
   } else {
     DPRINT1(0, "using simon system 0x%x", simon_system);
   }
   add_simonet_primitives(simon_system);
   return simon_system;
}

ficlVm* simon_vm_alloc()
{
  ficlSystem* simon_system = simonet_boot();
  ficlVm* vm = ficlSystemCreateVm(simon_system);
  simon_vm_init(vm, -1);
  return vm;
}

int simon_server(int port)
{
   ficlSystem* simon_system = NULL;
   fd_set active_fd_set, read_fd_set;
   struct sockaddr_in clientname;
   size_t size;
   int i;
   char reply[256];
   int returnValue = 0;
   ficlVm *vm[FD_SETSIZE];

   simon_system = simonet_boot();

   /* Create the socket and set it up to accept connections. */
   int sock = alloc_socket(port);
   if (listen(sock, 1) < 0)
   {
      perror("listen");
      exit(EXIT_FAILURE);
   }

   /* Initialize the set of active sockets. */
   FD_ZERO(&active_fd_set);
   FD_SET(sock, &active_fd_set);

   while (1)
   {
      /* Block until input arrives on one or more active sockets. */
      read_fd_set = active_fd_set;
      if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
      {
         perror("select");
         exit(EXIT_FAILURE);
      }

      /* Service all the sockets with input pending. 
       * :TODO: create a separate task for each connection !  This facilitates redirection of stdout.
       */
      for (i = 0; i < FD_SETSIZE; ++i) {
         if (FD_ISSET(i, &read_fd_set))
         {
            sprintf(reply, "bye\n");
            if (i == sock)
            {                   /* Connection request on original socket. */
               size = sizeof(clientname);
               int new = accept(sock,
                                (struct sockaddr *) &clientname,
                                &size);
               if (new < 0)
               {
                  perror("accept");
                  exit(EXIT_FAILURE);
               }
               fprintf(stderr,
                       "Server: connect %d from host %s, port %hu.\n",
                       i, inet_ntoa(clientname.sin_addr), ntohs(clientname.sin_port));
               FD_SET(new, &active_fd_set);
               vm[new] = ficlSystemCreateVm(simon_system);
               simon_vm_init(vm[new], new);
            }
            else
            {                   /* Data arriving on an already-connected socket. */
               if (process_client_request(i, vm[i]) < 0)
               {
                  fprintf(stderr,
                          "Server: closing connection %d from host %s, port %hu.\n",
                          i, inet_ntoa(clientname.sin_addr), ntohs(clientname.sin_port));
                  simon_vm_destroy(vm[i]);
                  close(i);
                  FD_CLR(i, &active_fd_set);
               }
            }
         }
      }
   }
   close(sock);
}

extern int calcSysClkTicks(int);

void nv_c_test()
{
   fprintf(stderr, "nv_c_test called\n");
}

void nv_c_test_1(int arg)
{
   fprintf(stderr, "nv_c_test(%d) called\n", arg);
}

void nv_c_test_2(int arg1, int arg2)
{
   fprintf(stderr, "nv_c_test(%d,%d) called\n", arg1, arg2);
}

/* call a C function from the forth interpreter */
#ifdef SIMONET_TEST
void nv_c_call(ficlVm * vm)
{
   fprintf(stderr, "nv_c_call not supported in test mode");
   abort();
}

void nv_c_spawn(ficlVm * vm)
{
   fprintf(stderr, "nv_c_spawn not supported in test mode");
   abort();
}

void nv_c_float_call(ficlVm * vm)
{
   fprintf(stderr, "nv_float_call not supported in test mode");
   abort();
}
#else

#if 0  // =============> TODO <==============
// :TODO: provide a wrapper that uses ioTaskStdSet or start the task in
//        halted state, ioTaskStdSet, and then start
int nv_c_wrapper(FUNCPTR func, int* arg)
{
  FUNCPTR start;               /* found entry point goes here */
  ioTaskStdSet(0, tid);
  int rv = (*start) (arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6],
		     arg[7], arg[8], arg[9]);
  free(args);
}
#endif

void nv_c_spawn(ficlVm * vm)
{
   FUNCPTR start;               /* found entry point goes here */
   unsigned char symType;
   char *fn;
   int i;
   unsigned arg[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

   /* start with a stack size of 3: function length, name, num args */
   FICL_STACK_CHECK(vm->dataStack, 3, 0);

   fprintf(stderr, "Server[0x%x] nv_c_spawn\n", (unsigned) vm);
   int function_name_len = ficlStackPopInteger(vm->dataStack);
   char *function_name = ficlStackPopPointer(vm->dataStack);
   int nargs = (int) ficlStackPopPointer(vm->dataStack);

   fprintf(stderr, "Server[0x%x] spawning %s with %d arguments\n", (unsigned) vm, function_name, nargs);

   FICL_STACK_CHECK(vm->dataStack, nargs, 0);
   for (i = 0; i < nargs; i++)
      arg[i] = ficlStackPopInteger(vm->dataStack);

   fn = malloc(strlen(function_name) + 1);
   strcpy(fn, function_name);
   if (symFindByName(sysSymTbl, fn, (char **) &start, &symType) == OK)
   {
      int millisecs = calcSysClkTicks(10);
      /* taskSpawn(name,priority,options,stacksize,entryAddress,arg1,..) */
      int tid = taskSpawn(function_name, 120, STD_TASKOPTIONS, XSTD_STACKSIZE,
                          start,
                          arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6],
                          arg[7], arg[8], arg[9]);
      taskDelay(millisecs);    // wait for some millisecs between polls
   }
   else
   {
      fprintf(stderr, "Server[0x%x] symbol %s not found\n", (unsigned) vm, function_name);
   }
   free(fn);
}

void nv_c_call(ficlVm * vm)
{
   FUNCPTR start;               /* found entry point goes here */
   unsigned char symType;
   char *fn;
   int i, rv;
   unsigned arg[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

   /* start with a stack size of 3: function length, name, num args */
   FICL_STACK_CHECK(vm->dataStack, 3, 0);

   fprintf(stderr, "Server[0x%x] nv_c_call\n", (unsigned) vm);
   int function_name_len = ficlStackPopInteger(vm->dataStack);
   char *function_name = ficlStackPopPointer(vm->dataStack);
   int nargs = (int) ficlStackPopPointer(vm->dataStack);
   printf("name_len: %d, func: '%s', nargs: %d\n", function_name_len, function_name, nargs);

   fprintf(stderr, "Server[0x%x] calling %s with %d arguments: ", (unsigned) vm, function_name, nargs);

   FICL_STACK_CHECK(vm->dataStack, nargs, 0);
   for (i = 0; i < nargs; i++)
   {
      arg[i] = ficlStackPopInteger(vm->dataStack);
      printf("%d ", arg[i]);
   }
   printf("\n");

   fn = malloc(strlen(function_name) + 1);
   strcpy(fn, function_name);
   // printf ("lkup: '%s'\n", fn);
   if (symFindByName(sysSymTbl, fn, (char **) &start, &symType) == OK)
   {
      printf("calling: '%s'\n", fn);
      int rv = (*start) (arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6],
                         arg[7], arg[8], arg[9]);
      printf("return value pushed on stack: %d\n",rv);
      ficlStackPushInteger(vm->dataStack, rv);
   }
   else
   {
      fprintf(stderr, "Server[0x%x] symbol %s not found\n", (unsigned) vm, function_name);
   }
   free(fn);
}

/* :TODO: provide a more general mechanism for dealing with FICL's
**        independent data and float stacks as a mechanism for passing
**        parameters to C functions and getting return values. 
*/
void nv_c_float_call(ficlVm * vm)
{
   FUNCPTR start;               /* found entry point goes here */
   unsigned char symType;
   char *fn;
   int i;
   float arg[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

   /* start with a stack size of 3: function length, name, num args */
   FICL_STACK_CHECK(vm->dataStack, 3, 0);

   fprintf(stderr, "Server[0x%x] nv_c_float_call\n", (unsigned) vm);
   int function_name_len = ficlStackPopInteger(vm->dataStack);
   char *function_name = ficlStackPopPointer(vm->dataStack);
   int nargs = (int) ficlStackPopPointer(vm->dataStack);

   fprintf(stderr, "Server[0x%x] calling %s with %d arguments: ", (unsigned) vm, function_name, nargs);

   FICL_STACK_CHECK(vm->floatStack, nargs, 0);
   for (i = 0; i < nargs; i++)
   {
      arg[i] = ficlStackPopFloat(vm->floatStack);
      printf("%f ", arg[i]);
   }
   printf("\n");

   fn = malloc(strlen(function_name) + 1);
   strcpy(fn, function_name);
   if (symFindByName(sysSymTbl, fn, (char **) &start, &symType) == OK)
   {
      float rv = (*start) (arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6],
                           arg[7], arg[8], arg[9]);
      ficlStackPushFloat(vm->floatStack, rv);
   }
   else
   {
      fprintf(stderr, "Server[0x%x] symbol %s not found\n", (unsigned) vm, function_name);
   }
   free(fn);
}
#endif

/* add simonet ficl primitives */
void add_simonet_primitives(ficlSystem* simon_system)
{
   ficlDictionary *dict = ficlSystemGetDictionary(simon_system);
   ficlWord *word = ficlDictionarySetPrimitive(dict, "nvs", nv_c_spawn,
                                               FICL_WORD_DEFAULT);
   ficlWord *word2 = ficlDictionarySetPrimitive(dict, "nv", nv_c_call, FICL_WORD_DEFAULT);
   ficlWord *wordf = ficlDictionarySetPrimitive(dict, "nvf", nv_c_float_call,
                                                FICL_WORD_DEFAULT);
}

int dump_fsi(ficlSystemInformation* fsi)
{
  printf("fsi:size = %d\n",fsi->size);
  printf("fsi:context = 0x%x\n",fsi->context);
  printf("fsi:dictionarySize = %d\n",fsi->dictionarySize);
  printf("fsi:stackSize = %d\n",fsi->stackSize);
  printf("fsi:textOut = 0x%x\n",fsi->textOut);
  printf("fsi:errorOut = 0x%x\n",fsi->errorOut);
  printf("fsi:environmentSize = %d\n",fsi->environmentSize);
}

#ifdef SIMONET_TEST

ficlUnsigned rf_spi_init(ficlVm * vm)
{
}

void simon_init(ficlVm * vm)
{
}

ficlSystem *get_simon_system()
{
   static ficlSystem *simon_system = NULL;
   return simon_system;
}

/**---------------------------
 * To test on a linux system without a hardware board on port 5555:
 *   make ns
 *   xterm -e $cwd/ns 5555 &
 *   echo 3 2 1 + + . | nc localhost 5555
 * which should return the value 6; This won't necessarily work on
 * a 64-bit system.
 */
int main(int argc, char **argv)
{
   int port = 5555;
   if (argc > 1)
      port = atoi(argv[1]);
   simon_server(port);
}
#else
/**
 * Spawn the server task with priority 150, floating point, and an expanded
 * stack size of 15K instead of the standard 7K.
 */
int simonet(int port, int stack)
{
   if (stack < 15000)           /* experience shows that less than 15000 is unhealthy */
      stack = 15000;
   if (port < 1)                /* default port */
      port = 5555;

   int task = taskSpawn("tSimonet", 150, VX_FP_TASK, stack, simon_server, port, 0,
                        0, 0, 0, 0, 0, 0, 0, 0);
   fprintf(stderr, "spawned simon server task %d\n", task);
   return task;
}
#endif
