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
/* fdfsplit.c - split header and data  */

#include <ctype.h>
#include <fcntl.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __STDC__
#include <unistd.h>
#endif
#if defined(__cplusplus) || defined(c_plusplus)
#include <osfcn.h>
#endif
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "boolean.h"
#include "error.h"
#include "crc.h"

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)

char *check_output (char *p_filename);
int write_output (char *src_file, int src_fd, char *dst_file, int dst_fd);
off_t get_filesize (char *name,int fd);
size_t get_length (size_t size);
int gluer (int offset, char *in_file, char *dat_file, char *dout_file);

extern int get_reply (char *prompt, char *reply, int max, int required);
#else

char *check_output();
int write_output();
off_t get_filesize();
size_t get_length();
int gluer();

extern int get_reply();
extern int getpagesize();
extern long strtol();

#endif

char *p_name = "gluer"; /* name of this program, for messages */
char  io_buffer[1024];  /* buffer for I/O operations */

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
char *check_output (char *p_filename)
#else
char *check_output (p_filename)
 char *p_filename;
#endif
{
   struct stat status;
   char ans[8];
   int  i;

   if (stat (p_filename, &status) == 0)
   {
      sprintf (io_buffer,
               "%s: output file '%s' already exists; over-write (y/n)?: ",
               p_name, p_filename);
      for (;;)
      {
         i = get_reply (io_buffer, ans, sizeof(ans), FALSE);

         if (i == EOF || i == 0 || ans[0] == '\0' || isspace(ans[0]))
            error_exit (1);
         else if (ans[0] == 'n' || ans[0] == 'N')
         {
            p_filename = (char *)NULL;   /* use stdout for data */
            break;
         }
         else if (ans[0] == 'y' || ans[0] == 'Y')
            break;
         else
            error ("Pleae type 'y' or 'n'!");
      }
   }
   return p_filename;

} /* end of function "check_output" */

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
int write_output (char *src_file, int src_fd, char *dst_file, int dst_fd)
#else
int write_output (src_file, src_fd, dst_file, dst_fd)
 char *src_file;
 int src_fd;
 char *dst_file;
 int dst_fd;
#endif
{
   int bytes;

   for (;;)
   {
      if ( (bytes = read (src_fd, io_buffer, sizeof (io_buffer))) == 0)
         break;
      else if (bytes == -1)
      {
         sys_error ("%s: can't read from '%s'", p_name, src_file);
         break;
      }
      if ( (bytes = write (dst_fd, io_buffer, bytes)) == -1)
      {
         sys_error ("%s: can't write to '%s'", p_name, dst_file);
         break;
      }
   }
   return bytes;

} /* end of function "write_output" */

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
off_t get_filesize (char *name, int fd)
#else
off_t get_filesize (name, fd)
 char *name;
 int   fd;
#endif
{
   off_t filesize = -1;        /* filesize */
   struct stat status;         /* structure of stat */

   /* Find the filesize */
   if (fstat (fd, &status) == -1)
      sys_error ("%s: can't fstat file '%s'", p_name, name);
   else if ( (filesize = status.st_size) == (off_t)0)
      error ("%s: file '%s' has 0 size", p_name, name);

   return (filesize);

}  /* end of function "get_filesize" */

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
size_t get_length (size_t size)
#else
size_t get_length (size)
 size_t size;
#endif
{
   size_t pagesize;

   /* Obtain the page size */
#if defined(__STDC__)
   pagesize = (size_t)sysconf(_SC_PAGESIZE);
#else
   pagesize = (size_t)getpagesize();
#endif

   /* Adjust the size to a multiple of the pagesize */
   if (size % pagesize)
      size += (pagesize - (size % pagesize) );
        
   return (size);
        
}  /* end of function "get_length" */

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
int get_hdrsize ( char  *in_map, size_t size)
#else
int get_hdrsize (in_map, size)
 char  *in_map;
 size_t size;
#endif
{
   int hdrsize;
   
   hdrsize = 0;
   /* Obtain the page size */
   while (*in_map != '\0')
   {
	in_map++;
	hdrsize++;
	if (hdrsize > size)
		return(-1);
   }
        
   return(hdrsize);
        
}  /* end of function "get_length" */

/****************************************************************************
 fdfsplit

 This function splits a header file and binary data.

 int fdfsplit ( char *in_file, char *hdr_file, char *dout_file  );

 INPUT ARGS:
   in_file    The name of the input fdf file;  if NULL, read from stdin.
   hdr_file    The name of the output header file. 
   dout_file    The name of the output data file. 
 OUTPUT ARGS:
   none
 RETURN VALUE:
   0           Operation succeeded.
   != 0        Operation failed: see ERRORS below.
 GLOBALS USED:
   p_name      Name of this function, for error messages.
 GLOBALS CHANGED:
   none
 ERRORS:
   return -1:
            Offset is less than zero.
            Name of a header file was not specified.
            Can't get size of specified header file, or file size is 0
               (see "get_filesize()").
            Can't get size of specified data file, or file size is 0
               (see "get_filesize()").
   return != 0:
            Can't open specified header file.
            "mmap()" failed for specified header file.
            Can't open specified data file.
            "mmap()" failed for specified data file.
            Can't open specified output file.
            Error occurred writing header to output file.
            Error occurred writing header terminator to output file.
            Error occurred writing data to output file.
 EXIT VALUES:
      1     In function "check_output()": user responded with carriage return,
            when asked to over-write an existing output file.
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

int fdfsplit ( char *in_file, char *hdr_file, char *dout_file)
{
   /**************************************************************************
   LOCAL VARIABLES:

   in_fd         Handle for header file.
   in_size       Size of header file, in bytes.
   in_len        Length of memory-map for header file.
   in_map        Pointer to memory-map for header file.
   chk_str        Array for building the data checksum and header terminator.
   chk_len        The length of the data checksum string and header terminator.
   chk_sum        Checksum for binary data.
   chk_text       "int checksum=": DDL variable name for checksum in header.
   hdr_fd         Handle for data file.
   hdr_size       Size of header file, in bytes.
   dout_fd         Handle for output file.
   */
   int    in_fd;
   off_t  in_size;
   size_t in_len;
   char  *in_map;

   int    hdr_fd;
   int  hdr_size;

   int    dout_fd;
   off_t  dout_size;
   char  *dout_map;

   if (in_file == (char *)NULL)
   {
      in_file = "(standard input)";
      in_fd   = 0;

#ifdef DEBUG
      error ("    in_file = '%s'", in_file);
#endif
   }
   /* open the header file, then find its size and memory-map length */
   else if ( (in_fd = open (in_file , O_RDONLY, 0)) == -1)
      return sys_error ("%s: can't open header file '%s' for reading", p_name,
                        in_file);

   if ( (in_size = get_filesize (in_file, in_fd)) <= 0)
      return -1;
   in_len = get_length ((size_t)in_size);

#ifdef DEBUG
   error ("    in_size = %d", in_size);
   error ("    in_len  = %d", in_len);
#endif

   /* map the header file into memory, then close it */
   if ( (in_map = (char *)mmap ((caddr_t)0, in_len, PROT_READ, MAP_SHARED,
                                 in_fd, (off_t)0)) == (char *)-1)
   {
      return sys_error ("%s: mmap header file '%s'", p_name, in_file);
   }
   close (in_fd);


   /* check for overwriting an existing output file; write to stdout if
      an existing file won't be over-written */
   if (hdr_file != (char *)NULL)
      hdr_file = check_output (hdr_file);

   /* open hdr file */
   if (hdr_file == (char *)NULL)
   {
      /* hdr_file = "(standard output)"; */
      /* hdr_fd = 1; */
      error ("%s: output header name is blank", p_name);
      return -1;
   }
   else if ( (hdr_fd = open (hdr_file, O_RDWR|O_TRUNC|O_CREAT, 0644)) == -1)
      return sys_error ("%s: can't open output file '%s' for writing", p_name,
                        hdr_file);
#ifdef DEBUG
   error ("    hdr_file = '%s'", hdr_file);
#endif

   /* check for overwriting an existing output file; write to stdout if
      an existing file won't be over-written */
   if (dout_file != (char *)NULL)
      dout_file = check_output (dout_file);

   /* open the output data file */
   if (dout_file == (char *)NULL)
   {
      error ("%s: output data name is blank", p_name);
      return -1;
   }
   else if ( (dout_fd = open (dout_file, O_RDWR|O_TRUNC|O_CREAT, 0644)) == -1)
      return sys_error ("%s: can't open output file '%s' for writing", p_name,
                        dout_file);
#ifdef DEBUG
   error ("    dout_file = '%s'", dout_file);
#endif

   /* get the header size */
   if ( (hdr_size = get_hdrsize(in_map, in_size)) <= 0)
   {
	error ("%s: hdr_size (%d) must be greater than 0", p_name, hdr_size);
	return -1;
   }

   /* write the header to the output file */
   if (write (hdr_fd, in_map, hdr_size) <= 0)
      return sys_error ("%s: can't write header to '%s'", p_name, hdr_file);
 
   dout_size = in_size - hdr_size - 1;	/* the 1 accounts for the NULL char */
   dout_map = in_map + hdr_size + 1;	/* seperating header from data      */

   /* write the data file to the output file */
   if (write (dout_fd, dout_map, dout_size) <= 0)
      return sys_error ("%s: can't write data to '%s'", p_name, dout_file);

   munmap (in_map, in_len);

   /* close the data and output files */
   if (hdr_fd != 1)
      close (hdr_fd);

   if (dout_fd != 1)
      close (dout_fd);

   return 0;

} /* end of function "gluer" */

#ifdef MAIN

void bad_cmd_line (void)
{
   error ("USAGE: %s [inputfdf_file] data_file header_file", p_name);
   error_exit (1);

} /* end of function "bad_cmd_line" */

int main (int argc, char *argv[])
{
   char  *in_file = (char *)NULL;
   char  *hdr_file = NULL;
   char  *dout_file = NULL;

   /* set a pointer to the name of this program */
   if ( (p_name = strrchr (argv[0], '/')) == (char *)NULL)
      p_name = argv[0];
   else
      ++p_name;

   /* skip past this command-line argument */
   --argc; ++argv;

   /* decode the command-line arguments */
   if (argc <= 0)
      bad_cmd_line();

   /* set up from the rest of the command-line arguments */
   switch (argc)
   {
      case 3:      /* header, data and output files specified */
         in_file = argv[0];
         dout_file = argv[1];
         hdr_file = argv[2];
         break;

      case 2:      /* only header and data files specified */
         dout_file = argv[0];
         hdr_file = argv[1];
         break;

      default:
         bad_cmd_line();
   }
#ifdef DEBUG
   error ("    in_file = '%s'", in_file);
#endif

   return (fdfsplit ( in_file, hdr_file, dout_file) );

} /* end of function "main" */

#endif
