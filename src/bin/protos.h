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
/* Only the prototypes of functions designed to be globally visible are found
   here.  Local functions are prototyped only in their own modules.
*/

#ifdef __STDC__

void open_params_in(FILE ** fpp, char * filename, int required);
void errmsg(int rc,
	    char * file,
	    int line,
	    long extra);
void cleanup(int retcode);
int read_rawfid(int bytes);
int read_rawfid_separated(int bytes,
			  int separation);
unsigned char * expand(unsigned char * old_data,
		       int oldsize,
		       int newsize,
		       int elements);
void reverse_bytes(unsigned char * old_data,
		   int elements,
		   int size);
unsigned char * shuffle(unsigned char * old_data,
			int elements,
			int size);
void negate(unsigned char * old_data,
	    int elements,
	    int size,
	    int offset);
void alternate(unsigned char * old_data,
	       int elements,
	       int size);
void zero(unsigned char * old_data,
	  int elements,
	  int size,
	  int offset);
void scale(unsigned char * old_data,
	   int elements,
	   int size,
	   int offset,
	   int is_float,
	   double factor);
void vax_float(unsigned char * old_data,
	       int elements);
void ieee_float(unsigned char * old_data,
		int elements);
long bruker_file(long type);
void nmr1_file(long type);
long cmx_file(long type);
long smis_file(long type);
long m100_file(long type);
long unity_file(long type);

void experimental_method(unsigned char ** old_datap,
			 int elements,
			 int size,
			 long type,
			 long extype,
			 int exp1,
			 int exp2,
			 int exp3);

#else /* __STDC__ */

void errmsg();
void cleanup();
int read_rawfid();
int read_rawfid_separated();
unsigned char * expand();
void reverse_bytes();
unsigned char * shuffle();
void negate();
void alternate();
void zero();
void scale();
void vax_float();
void ieee_float();
long bruker_file();
void nmr1_file();
long cmx_file();
long smis_file();
long m100_file();
long unity_file();

void experimental_method();

#endif /* __STDC__ */
