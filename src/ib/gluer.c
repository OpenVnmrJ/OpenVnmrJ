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
/* gluer.c - put header and data together */

#include <ctype.h>
#include <fcntl.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

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
#include "data.h"

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)

char *check_output (char *p_filename);
int write_output (char *src_file, int src_fd, char *dst_file, int dst_fd);
off_t get_filesize (char *name,int fd);
size_t get_length (size_t size);
int gluer (int align, off_t offset, char *template_file,
	   char *hdr_file, char *dat_file, char *out_file, int byteswap);

extern int get_reply (char *prompt, char *reply, int max, int required);
#else

char *check_output();
int write_output();
off_t get_filesize();
size_t get_length();
int gluer();
extern int getpagesize();
extern int get_reply();

#endif

char *p_name = "fdfgluer"; /* name of this program, for messages */
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

int write_template (off_t offset, char *src_template, char *dst_file, int dst_fd, int byteswap)
{
   int bytes = 0;
   char *src_prefix;
   char *src_suffix;
   char *src_file;
   int src_fd;
   int n;
   int tlen;

   tlen = strlen(src_template);
   src_file = (char *)malloc(tlen + 5);
   if (!src_file){
       sys_error("%s: cannot malloc() memory for filename (%s)",
		 p_name, src_template);
       return -1;
   }
   src_prefix = src_template;
   src_suffix = strchr(src_template, '#');
   if (src_suffix){
       *(src_suffix)++ = '\0';	/* Splits template into two strings */
   }else{
       src_suffix = src_template + tlen; /* Points to NULL */
   }
   for (n=1; n<10000; n++){
       sprintf(src_file, "%s%d%s", src_prefix, n, src_suffix);
       src_fd = open(src_file, O_RDONLY, 0);
       if (src_fd == -1){
	   break;
       }
       if (lseek(src_fd, offset, SEEK_SET) == -1){
	   sys_error("%s: lseek() failed in %s",
		     p_name, src_file);
	   return -1;
       }
       while (1){
	   if ( (bytes = read(src_fd, io_buffer, sizeof (io_buffer))) == 0){
	       break;
	   }else if (bytes == -1){
	       sys_error ("%s: can't read from '%s'", p_name, src_file);
	       break;
	   }
#ifdef LINUX
           if (byteswap)
           {
              int *swapPtr;
              int  swapWords;
              int  swapCnt;

              swapPtr = (int *) &io_buffer[0];
              swapWords = bytes / sizeof(float);
              for (swapCnt = 0; swapCnt < swapWords; swapCnt++)
              {
                 *swapPtr = ntohl( *swapPtr );
                 swapPtr++;
              }
           }
#endif
	   if ( (bytes = write (dst_fd, io_buffer, bytes)) == -1){
	       sys_error ("%s: can't write to '%s'", p_name, dst_file);
	       break;
	   }
       }
       close(src_fd);
   }
   return bytes;		/* Returns 0 on success */

} /* end of function "write_template" */

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

/****************************************************************************
 gluer

 This function concatenates a header file and binary data, with the data
 aligned as requested.

 int gluer (int align, char *hdr_file, char *dat_file, char *out_file);

 INPUT ARGS:
   align      The alignment of the data section; must be > 0.
   hdr_file    The name of the file containing the header; must be present.
   dat_file    The name of the file containing the binary data to be appended
               to the header; if NULL, read from stdin.
   out_file    The name of the file containing the concatenated and aligned
               binary data; if NULL, write to stdout.
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
            Align is less than zero.
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

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
static char *get_pad(int align, int hdr_len)
#else
static char *get_pad(align, hdr_len)
  int align;
  int hdr_len;
#endif
{
    static char buf[1024];
    char *pc;
    int pad_cnt;

    /* Find the required number of padding characters for the terminator string
     * for this header file and alignment.  Extra 3 chars are "\f\n" and
     * the header-terminating NULL. */
    pad_cnt = ((int)hdr_len + 3) % align;
    pad_cnt = (align - pad_cnt) % align;
    
    /* Put in the string to terminate listings by "more" */
    (void)strcpy (buf, "\f\n");
    pc = buf + 2;
    
    /* Put in padding */
    if (pad_cnt){
	(void)memset (pc, '\n', pad_cnt);
	pc += pad_cnt;
    }
    
    /* Terminating NULL */
    *pc = '\0';
    return buf;
}

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
static int read_trace(int fd, struct datafilehead fhead, int itrace,
		      char *buf, int *ct, int *scale)
#else
static int read_trace(fd, fhead, itrace, buf, ct, scale)
  int fd;
  struct datafilehead fhead;
  int itrace;
  char *buf;
  int *ct;
  int *scale;
#endif
{
    int err;
    int rtn;
    int skip_blks;		/* Nbr of blocks before FID we want */
    int skip_traces;		/* Nbr of traces to skip at front of block */
    struct datablockhead dhead;
    off_t fOffset;
    
    /* Go to the correct block */
    skip_blks = itrace / fhead.ntraces;
    skip_traces = itrace % fhead.ntraces;
    fOffset = sizeof(fhead);
    fOffset += (off_t) skip_blks * (off_t) fhead.bbytes;
    err = lseek(fd, fOffset, SEEK_SET);
    if (err < 0){
	sys_error("%s: Lseek to Vnmr data block failed", p_name);
	return 0;
    }
    /* Read the block header */
    err = read(fd, &dhead, sizeof(dhead));
    if (err != sizeof(dhead)){
	sys_error("%s: Failed reading Vnmr block header", p_name);
	return 0;
    }
    if (!(dhead.status & S_DATA)){
	sys_error("%s: Not enough data in Vnmr data file", p_name);
	return 0;
    }
    /* Read the data */
    if (skip_traces){
        fOffset = (off_t) skip_traces * (off_t) fhead.tbytes;
	err = lseek(fd, fOffset, SEEK_CUR);
	if (err < 0){
	    sys_error("%s: Lseek to Vnmr data trace failed", p_name);
	    return 0;
	}
    }
    rtn = read(fd, buf, fhead.tbytes);
    *ct = dhead.ctcount;
    *scale = dhead.scale;
    return rtn;
}

int gluer (int align, off_t offset, char *template_file,
	   char *hdr_file, char *dat_file, char *out_file, int byteswap)
{
   /**************************************************************************
   LOCAL VARIABLES:

   hdr_fd         Handle for header file.
   hdr_size       Size of header file, in bytes.
   hdr_len        Length of memory-map for header file.
   hdr_map        Pointer to memory-map for header file.
   chk_str        Array for building the data checksum and header terminator.
   chk_len        The length of the data checksum string and header terminator.
   chk_sum        Checksum for binary data.
   chk_text       "int checksum=": DDL variable name for checksum in header.
   pad_cnt        Number of pad characters needed for header terminator, to
                  align the binary data as requested.
   dat_fd         Handle for data file.
   dat_size       Size of data file, in bytes.
   dat_len        Length of memory-map for data file.
   dat_map        Pointer to memory-map for data file.
   out_fd         Handle for output file.
   */
   int    hdr_fd;
   off_t  hdr_size;
   size_t hdr_len;
   char  *hdr_map;

   char   chk_str[256];
   int    chk_len;
   tcrc   chk_sum;
   char  *chk_text = "int checksum =";

   int    pad_cnt;

   int    dat_fd;
   off_t  dat_size = 0;
   size_t dat_len = 0;
   char  *dat_map = NULL;

   int    out_fd;

   if (align < 1)
   {
      error ("%s: alignment (%d) must be greater than 0", p_name, align);
      return -1;
   }
   if (hdr_file == (char *)NULL)
   {
      error ("%s: must have the name of a header file", p_name);
      return -1;
   }
   /* open the header file, then find its size and memory-map length */
   if ( (hdr_fd = open (hdr_file , O_RDONLY, 0)) == -1)
      return sys_error ("%s: can't open header file '%s' for reading", p_name,
                        hdr_file);

   if ( (hdr_size = get_filesize (hdr_file, hdr_fd)) <= 0)
      return -1;
   hdr_len = get_length ((size_t)hdr_size);

#ifdef DEBUG
   error ("    hdr_size = %d", hdr_size);
   error ("    hdr_len  = %d", hdr_len);
#endif

   /* map the header file into memory, then close it */
   if ( (hdr_map = (char *)mmap ((caddr_t)0, hdr_len, PROT_READ, MAP_SHARED,
                                 hdr_fd, (off_t)0)) == (char *)-1)
   {
      return sys_error ("%s: mmap header file '%s'", p_name, hdr_file);
   }
   close (hdr_fd);
   dat_fd   = 0;

   /* open the data file, if needed */
   if (template_file || !dat_file)
   {
       sprintf (chk_str, "%s 0;\n", chk_text);
   }
   if (!template_file && !dat_file)
   {
      dat_file = "(standard input)";

#ifdef DEBUG
      error ("    dat_file = '%s'", dat_file);
#endif
   }
   else if ( !template_file && (dat_fd = open (dat_file , O_RDONLY, 0)) == -1)
   {
      return sys_error ("%s: can't open data file '%s' for reading", p_name,
                        dat_file);
   }
   else if (!template_file)
   {
      /* find the data file's size and memory-map length */
      if ( (dat_size = get_filesize (dat_file, dat_fd)) <= 0)
         return -1;
      dat_len = get_length ((size_t)dat_size);
    
#ifdef DEBUG
   error ("    dat_file = '%s'", dat_file);
   error ("    dat_size = %d", dat_size);
   error ("    dat_len  = %d", dat_len);
#endif
 
      /* map the data file into memory */
      if ( (dat_map = (char *)mmap ((caddr_t)0, dat_len, PROT_READ, MAP_SHARED,
                                    dat_fd, (off_t)0)) == (char *)-1)
      {
         return sys_error ("%s: mmap data file '%s'", p_name, dat_file);
      }
      /* compute the check-sum, then build the string for the header */
      chk_sum = addbfcrc (dat_map, dat_size);

      sprintf (chk_str, "%s %010lu;\n", chk_text, chk_sum);
   }
   chk_len = strlen (chk_str);

#ifdef DEBUG
   error ("    chk_len  = %d", chk_len);
#endif

   /* Find the required number of padding characters for the terminator string
    * for this header file and alignment.  Extra 3 chars are "\f\n" and
    * the header-terminating NULL. */
   pad_cnt = ((int)hdr_size + chk_len + 3) % align;
   pad_cnt = (align - pad_cnt) % align;

   /* Put in the string to terminate listings by "more" */
   (void)strcpy (chk_str + chk_len, "\f\n");
   chk_len += 2;

   /* Put in padding */
   if (pad_cnt){
       (void)memset (chk_str + chk_len, '\n', pad_cnt);
       chk_len += pad_cnt;
   }

   /* Terminating NULL */
   *(chk_str + chk_len) = '\0';
   chk_len++;			/* Total length includes NULL */

#ifdef DEBUG
   {
      int i;

      fprintf (stderr, "    checksum = '");
      for (i = 0; i < chk_len; ++i)
      {
         switch (chk_str[i])
         {
            case '\n':
               fprintf (stderr, "\\n");
               break;
            case '\f':
               fprintf (stderr, "\\f");
               break;
            case '\0':
               fprintf (stderr, "\\0");
               break;
            default:
               if (isprint (chk_str[i]))
                  fprintf (stderr, "%c", chk_str[i]);
               else
                  fprintf (stderr, "0x%02X", chk_str[i]);
               break;
         }
      }
      fprintf (stderr, "'\n");
      error ("    chk_len  = %d", chk_len);
      fprintf (stderr, "    total    = %d\n", hdr_size+chk_len);
   }
#endif

   /* check for overwriting an existing output file; write to stdout if
      an existing file won't be over-written */
   if (out_file != (char *)NULL)
      out_file = check_output (out_file);

   /* open the output file */
   if (out_file == (char *)NULL)
   {
      out_file = "(standard output)";
      out_fd = 1;
   }
   else if ( (out_fd = open (out_file, O_RDWR|O_TRUNC|O_CREAT, 0644)) == -1)
      return sys_error ("%s: can't open output file '%s' for writing", p_name,
                        out_file);
#ifdef DEBUG
   error ("    out_file = '%s'", out_file);
#endif
   /* write the header and terminator string to the output file */
   if (write (out_fd, hdr_map, hdr_size) <= 0)
      return sys_error ("%s: can't write header to '%s'", p_name, out_file);
 
   if (write (out_fd, chk_str, chk_len) <= 0)
      return sys_error ("%s: can't write header end to '%s'", p_name, out_file);

   /* write the data file(s) to the output file */
   if (template_file)
   {
       if (write_template(offset, template_file, out_file, out_fd, byteswap) < 0){
	   return -1;
       }
   }
   else if (dat_fd == 0)
   {
      if (write_output (dat_file, dat_fd, out_file, out_fd) < 0)
         return (-1);
   }
   else if (write (out_fd, dat_map, dat_size) <= 0)
   {
      return sys_error ("%s: can't write data to '%s'", p_name, dat_file);
   }

   munmap (hdr_map, hdr_len);

   /* close the data and output files */
   if (dat_fd != 0)
   {
      munmap (dat_map, dat_len);
      close (dat_fd);
   }
   if (out_fd != 1)
      close (out_fd);

   return 0;

} /* end of function "gluer" */

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
int vgluer(int align, char *vnmr_fname, int ntraces, char *outfile_tmpl,
	   char *hdr_file)
#else
int vgluer(align, vnmr_fname, ntraces, outfile_tmpl, hdr_file)
  int   align;			/* Wordsize for memory alignment (>0) */
  char *vnmr_fname;		/* Input Vnmr format data file */
  int ntraces;			/* Number of traces / output file */
  char *outfile_tmpl;		/* Serial # will be added, e.g. "data#.fdf" */
#endif
{
    char *databuf = 0;
    char *out_file = 0;
    char *hdr_map;
    char *out_prefix;
    char *out_suffix;
    char *pad_str;
    char *pc;
    char *chk_text = "int checksum =";
    char *ct_text = "int ct =";
    char *nfile_text = "int nfile =";
    char *scale_text = "int scale =";
    char info_str[1024];
    int bytes;
    int ct;
    int dat_fd;
    int end;
    int hdr_fd;
    int hdr_size;
    int ifid;
    int ifile;
    int len;
    int nbytes;			/* Bytes / output file */
    int nfiles;			/* # of files to write out */
    int out_fd;
    int scale;
    int tlen;
    size_t hdr_len;
    tcrc chk_sum;
    struct datafilehead file_hdr;

    if (align < 1)
    {
	error ("%s: alignment (%d) must be greater than 0", p_name, align);
	return -1;
    }
    if (hdr_file == (char *)NULL || *hdr_file == '\0')
    {
	error ("%s: must have the name of a header file", p_name);
	return -1;
    }
    /* open the header file, then find its size and memory-map length */
    if ( (hdr_fd = open (hdr_file , O_RDONLY, 0)) == -1){
	return sys_error ("%s: can't open header file '%s' for reading", p_name,
			  hdr_file);
    }
    
    if ( (hdr_size = get_filesize (hdr_file, hdr_fd)) <= 0){
	return -1;
    }
    hdr_len = get_length ((size_t)hdr_size);
    
#ifdef DEBUG
    error ("    hdr_size = %d", hdr_size);
    error ("    hdr_len  = %d", hdr_len);
#endif
    
    /* map the header file into memory, then close it */
    if ( (hdr_map = (char *)mmap ((caddr_t)0, hdr_len, PROT_READ, MAP_SHARED,
				  hdr_fd, (off_t)0)) == (char *)-1)
    {
	return sys_error ("%s: mmap header file '%s'", p_name, hdr_file);
    }
    close (hdr_fd);
    
    /* Open the Vnmr format data file */
    dat_fd = open(vnmr_fname, O_RDONLY, 0);
    if (dat_fd == -1){
	return sys_error("%s: can't open data file '%s' for reading",
			 p_name, vnmr_fname);
    }
    
    /* Get the Vnmr File Header info */
    bytes = read(dat_fd, &file_hdr, sizeof(file_hdr));
    if (bytes == -1){
	close(dat_fd);
	return sys_error("%s: Cannot read Vnmr File Header from %s",
			 p_name, vnmr_fname);
    }
    
    /* VALIDATE HEADER INFO */

    
    /* Get a data buffer */
    if (!ntraces){
	ntraces = file_hdr.nblocks * file_hdr.ntraces;
    }
    nfiles = file_hdr.nblocks * file_hdr.ntraces / ntraces;
    nbytes = ntraces * file_hdr.tbytes;	/* Data bytes per output file */
    databuf = (char *)malloc(nbytes);
    if (!databuf){
	sys_error("%s: Cannot malloc memory for data buffer", p_name);
	return -1;
    }
    
    /* Prepare to construct output file names */
    tlen = strlen(outfile_tmpl);
    out_file = (char *)malloc(tlen + 5);
    if (!out_file){
	sys_error("%s: cannot malloc() memory for filename (%s)",
		  p_name, outfile_tmpl);
	return -1;
    }
    out_prefix = outfile_tmpl;
    out_suffix = strchr(outfile_tmpl, '#');
    if (out_suffix){
	*(out_suffix)++ = '\0';	/* Splits template into two strings */
    }else{
	out_suffix = outfile_tmpl + tlen; /* Points to NULL */
    }
    
    /* Loop over all output files */
    for (ifile=1, ifid=0; ifile <= nfiles; ifile++){
	/* Open output file */
	sprintf(out_file,"%s%04d%s", out_prefix, ifile, out_suffix);
	out_fd = open(out_file, O_RDWR | O_TRUNC | O_CREAT, 0666);
	if (out_fd == -1){
	    sys_error("%s: Cannot open \"%s\" for writing",
		      p_name, out_file);
	    return -1;
	}

	/* Get the data for this output file */
	end = ifid + ntraces;
	for (pc=databuf; ifid < end; ifid++, pc += file_hdr.tbytes){
	    bytes = read_trace(dat_fd, file_hdr, ifid, pc, &ct, &scale);
	    if (bytes != file_hdr.tbytes){
		sys_error("%s: Cannot read trace #%d from \"%s\"",
			  p_name, ifid, vnmr_fname);
		return -1;
	    }
	}

	/* Put in the extra header info */
	pc = info_str;
	sprintf(pc,"%s %06d;\n%s %06d;\n%s %06d;\n",
		nfile_text, ifile,
		ct_text, ct,
		scale_text, scale);
	pc += strlen(pc);

	/* Make checksum string for header */
	chk_sum = addbfcrc (databuf, nbytes);
	sprintf (pc, "%s %010lu;\n", chk_text, chk_sum);
	pc += strlen(pc);

	/* Get header padding and terminating string */
	pad_str = get_pad(align, hdr_size + strlen(info_str));

	/* Write out the header */
	bytes = write(out_fd, hdr_map, hdr_size);
	if (bytes < hdr_size){
	    sys_error("%s: Cannot write header to \"%s\"", p_name, out_file);
	    return -1;
	}
	len = (int)strlen(info_str);
	bytes = write(out_fd, info_str, len);
	if (bytes < len){
	    sys_error("%s: Cannot write hdr info to \"%s\"", p_name, out_file);
	    return -1;
	}
	len = (int)strlen(pad_str) + 1; /* Write the terminating NULL too */
	bytes = write(out_fd, pad_str, len);
	if (bytes < len){
	    sys_error("%s: Cannot write header padding to \"%s\"",
		      p_name, out_file);
	    return -1;
	}

	/* Write the data */
	bytes = write(out_fd, databuf, nbytes);
	if (bytes < nbytes){
	    sys_error("%s: Cannot write data to \"%s\"", p_name, out_file);
	    return -1;
	}
	close(out_fd);
    }
    close(dat_fd);
    if (databuf) free(databuf);
    if (out_file) free(out_file);
    return 0;
}


#ifdef MAIN

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
void bad_cmd_line (void)
#else
void bad_cmd_line()
#endif
{
   error ("USAGE: %s [-align] header_file [data_file [output_file]]",
	  p_name);
   error ("       %s -infiles template [-offset n] [-align] header_file",
	  p_name);
   error ("       %s -vnmrfile fname -outfiles template [-traces n] [-align] header_file",
	  p_name);
   error_exit (1);

} /* end of function "bad_cmd_line" */

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
int main (int argc, char *argv[])
#else
int main (argc, argv)
 int   argc;
 char *argv[];
#endif
{
   int    rtn = 0;
   int    align = sizeof (double);
   off_t  offset = 0;
   int    ntraces = 0;
   int    byteswap = 0;
   char  *outfile_tmpl = (char *)NULL;
   char  *vnmr_fname = (char *)NULL;
   char	 *template_file = (char *)NULL;
   char  *hdr_file = NULL;
   char  *dat_file = (char *)NULL;
   char  *out_file = (char *)NULL;

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

   /* check for options */
   while (argc && *argv[0] == '-')
   {
      if (strcasecmp(*argv, "-infiles") == 0)
      {
	  /* Read in a file name template */
	  --argc; ++argv;
	  if (argc <= 0){
	      bad_cmd_line();
	  }
	  template_file = *argv;
      }
      else if (strcasecmp(*argv, "-offset") == 0)
      {
	  /* Read in an offset value */
	  --argc; ++argv;
	  if (argc <= 0 || strspn(*argv, "0123456789") != strlen(*argv)){
	      bad_cmd_line();
	  }
	  offset = (off_t)strtol(*argv, (char **)NULL, 10);
      }
      else if (strcasecmp(*argv, "-outfiles") == 0)
      {
	  /* Read in an output filename template */
	  --argc; ++argv;
	  if (argc <= 0){
	      bad_cmd_line();
	  }
	  outfile_tmpl = *argv;
      }
      else if (strcasecmp(*argv, "-vnmrfile") == 0)
      {
	  /* Read data from Vnmr format file (stdin not allowed) */
	  --argc; ++argv;
	  if (argc <= 0){
	      bad_cmd_line();
	  }
	  vnmr_fname = *argv;
      }
      else if (strcasecmp(*argv, "-byteswap") == 0)
      {
	  /* Read in value for byteswap flag */
	  --argc; ++argv;
	  if (argc <= 0){
	      bad_cmd_line();
	  }
	  byteswap = (int)strtol(*argv, (char **)NULL, 10);
      }
      else if (strcasecmp(*argv, "-ntraces") == 0)
      {
	  /* Read in an offset value */
	  --argc; ++argv;
	  if (argc <= 0 || strspn(*argv, "0123456789") != strlen(*argv)){
	      bad_cmd_line();
	  }
	  ntraces = (int)strtol(*argv, (char **)NULL, 10);
      }
      else
      {
	  /* No valid code--must be an alignment value */
	  if (strspn(*argv+1, "0123456789") != strlen(*argv+1)){
	      bad_cmd_line();
	  }
	  /* get the alignment value */
	  align = (int)strtol (argv[0] + 1, (char **)NULL, 10);
      }

      /* skip past this command-line argument */
      --argc; ++argv;
   }

   /* set up from the rest of the command-line arguments */
   switch (argc)
   {
      case 3:      /* header, data and output files specified */
         out_file = argv[2];

      case 2:      /* only header and data files specified */
         dat_file = argv[1];

      case 1:      /* only header file specified */
         hdr_file = argv[0];
         break;

      default:
         bad_cmd_line();
   }
   if (offset && !template_file){
       fprintf(stderr,"\"-offset\" option only supported with \"-infiles\"\n");
       bad_cmd_line();
   }
#ifdef DEBUG
   error ("%s: align = %d", p_name, align);
   error ("    hdr_file = '%s'", hdr_file);
#endif

   if (vnmr_fname){
       rtn = vgluer(align, vnmr_fname, ntraces, outfile_tmpl, hdr_file);
   }else{
       rtn = gluer(align, offset, template_file, hdr_file, dat_file, out_file, byteswap);
   }
   return rtn;
} /* end of function "main" */

#endif
