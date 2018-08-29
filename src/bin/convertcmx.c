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
static char Version_ID[] = "convert v2.75 STM 7/13/98";
/* convert:  an NMR data file conversion utility.  Reads CMX, Bruker, and
   NMR1 data files (currently 1-D only for Bruker and NMR1).  Writes Varian's
   SREAD import/export format.  In the future, may write native Varian. 
   Various manipulations are done on the binary fid data to bring it into 32-
   bit integer (shuffled) format as would be understood by a Sun, so no
   further conversions are required of VNMR; the stext file reflects this. 
   (Bruker data may not be corrected for its sequential acquisition scheme,
   though.)  Where possible, parameters are translated into their Varian
   equivalent names and definitions.  In many cases, if a foreign parameter
   name does not conflict with a Varian parameter, it is imported without
   translation as well.
   
   Command line options control what input format is expected and what output
   is produced.  The first and second command line arguments which do not
   begin with - are used as the input and output file-or directory-names
   (which default to standard input and standard output if they are supposed
   to be files, or "convert.in" and "convert.out" if directories.  See usage()
   for options.
*/
/* Revision history:
   0.1	disjointed Unix programs to translate Bruker data (8/89).
   0.9	combined on Sun, filter interface (9/89)
   1.0	first Amiga version, command line options, Bruker/NMR1 (9/89)
   1.1	bug fixes and changes for Sun compiler (9/89)
   1.2	added CMX conversion (10/89)
   1.3	added -a, fixed CMX (10/89)
   2.0	added 2-D support for CMX, internal restructuring for multi-fid
		experiments (11/1/89)
   2.1	rewrote parameter parsing in CMX (4/27/90)
   2.2	handles floating point CMX data as well as integer; protocol for
		alteration of fid type based on parameters (5/24/90)
   2.21	added qualifiers to string parameters for correct parsing by sread
   2.22 changed defaults processing for "type" (6/7/90)
   2.23 cosmetic changes before distribution (6/19/91)
   2.23 typos (5/12/92)
   2.30 added simple SMIS conversion (2/93)
   2.31	changed order of operations in convert(), and scale() floats as floats;
		allow arrayed and 2-D SMIS data; changed stext output slightly
   2.32	better scaling of SMIS parameters, and basenaming of strings
   2.33	SMIS bugfix
   2.34	Bruker rawread; ALL_READ cosmetic
   2.35	replace Bruker defaults with ones for Bruknet-to-Sun data
   2.36	final "fix" of Bruker sequential data
   2.40	"final" revised Bruker translation
   2.41	allow oversized SMIS VAR_ARRAYs, fixed SMIS basename bug
   2.42	added -cw flag to distinguish CMXW from Intel-based CMX data 
   2.50 added -M flag for M-100 conversion
   2.60 added -cs flag for CMX Spinsight conversion
   2.62	added read_rawfid_separated for Spinsight arrays
   2.70	Unity fid file input/output and experimental method interface
   2.71	casting changes to please Sun C compiler
   2.72 better handling of Spinsight array vs. 2-D question
   2.73	observe Spinsight use_array value
   2.74	Y2K handling of Spinsight dates
   2.75	bugfix of Y2K handling of Spinsight dates
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "vdata.h"

#include "convert.h"

#ifdef EXPTL
#include "exptl.h"
#endif /* EXPTL */

#include "protos.h"

/* local prototypes */
#ifdef __STDC__

int main(int argc,
	 char ** argv);
void usage(void);
int parse_commandline(int argc,
		      char * argv[],
		      int * file1,
		      int * file2);
void open_inout(int argc,
		char * argv[],
		int inarg,
		int outarg,
		long type);
FILE * open_file_in_dir(char * file,
			char * dir,
			char * mode);
long read_params(long in_type);
void read_fid(long type,
	      long trace);
void convert_data(long in_type);
void write_data(long in_type);
void write_fhead(struct datafilehead * head);
void write_bhead(struct datablockhead * head);
void write_rawfid(int bytes);

#else /* __STDC__ */

int main();
void usage();
int parse_commandline();
void open_inout();
FILE * open_file_in_dir();
long read_params();
void read_fid();
void convert_data();
void write_data();
void write_fhead();
void write_bhead();
void write_rawfid();

#endif /* __STDC__ */

FILE * fid_in, * params_in, * text_in;
FILE * fid_out, * params_out, * text_out;
FILE * temp_out = NULL;
char * in_dirname;
char * out_dirname;
unsigned char * in_header;
unsigned char * in_data;
struct datafilehead in_fhead, out_fhead;
struct datablockhead in_bhead, out_bhead;
double realfactor, imagfactor;
long blind_skip, blind_length;
long exptl_type;
long exptl_p1, exptl_p2, exptl_p3;

int main(argc, argv)
int argc;
char *argv[];
{
    int file1, file2;
    long type;
    long i, fids;

    type = parse_commandline(argc, argv, &file1, &file2);
    open_inout(argc, argv, file1, file2, type);

    type = read_params(type);

    if (type & (OUT_VFID | OUT_SDATA)) {
	fids = in_fhead.nblocks * in_fhead.ntraces;
	for (i = 0; i < fids; i++) {
	    read_fid(type, i);
	    convert_data(type);
	    write_data(type);
	}
    }

    cleanup(RC_SUCCESS);
}

void usage() {
    fprintf(stderr,"Usage:  convert [options] [infile [outfile]]\n\n");
    fprintf(stderr,"Input file options are:\n");
    fprintf(stderr,"-b	Bruker input file\n");
    fprintf(stderr,"-bb s n	Bruker blind read (skip s bytes, then read n bytes as a fid)\n");
    fprintf(stderr,"-c	CMX input directory (Intel-based data from \"spec\")\n");
    fprintf(stderr,"-cw	CMX input directory (Sun-based data from \"CMXW\")\n");
    fprintf(stderr,"-cs	CMX input directory (Sun-based data from \"Spinsight\")\n");
    fprintf(stderr,"-i	SMIS input file\n");
    fprintf(stderr,"-m	NMR1 input file\n");
    fprintf(stderr,"-M	Chemagnetics M-series input file\n");
    fprintf(stderr,"-s	Varian SREAD input directory (not implemented)\n");
    fprintf(stderr,"-v	Varian Unity input directory (partially implemented)\n\n");
    fprintf(stderr,"Output file options are:\n");
    fprintf(stderr,"-os	output Varian SREAD directory (-osd, -osp give just the data\n");
    fprintf(stderr,"	(sdata) or parameter (stext) components)\n");
    fprintf(stderr,"-ov	output Varian Unity directory (-ovd, -ovp, -ovt give just the\n");
    fprintf(stderr,"	data (fid), parameter (procpar), or text (text) components)\n\n");
    fprintf(stderr,"Other options are:\n");
    fprintf(stderr,"--	negate the imaginaries, causing spectrum reverse\n");
    fprintf(stderr,"-+	do not negate the imaginaries (to override a default)\n");
    fprintf(stderr,"-n	write fid data without headers (really the same as -osd)\n");
    fprintf(stderr,"-a	(alternate) negate both real and imaginary parts of every second\n");
    fprintf(stderr,"	complex point (\"Redfield trick\", for Bruker data)\n");
    fprintf(stderr,"-A	do not do Redfield trick (to override a default)\n");
    fprintf(stderr,"-r	byte-reverse input words (e.g. 0123 -> 3210, 012 -> 210)\n");
    fprintf(stderr,"-R	do not do byte-reverse (to override a default)\n");
    fprintf(stderr,"-fr number	multiply all reals by number\n");
    fprintf(stderr,"-fi number	multiply all imaginaries by number\n");
#ifdef EXPTL
    fprintf(stderr,"-xz arg...  experimental method \"z\"; args (up to 3) vary with method\n");	
#endif /* EXPTL */
    fprintf(stderr,"-#	print the version number on stderr\n\n");
    fprintf(stderr,"One input file option and one output file option are required (no default manufacturers!).\n");
    fprintf(stderr,"Input filename and output filename are required if a directory is expected.\n");
    fprintf(stderr,"If a plain file is expected, they default to standard input and output.\n");
    fprintf(stderr,"Default conversions are:\n");
    fprintf(stderr,"	Bruker:     expand 24-bit to 32-bit integers, spectrum reverse, Redfield\n");
    fprintf(stderr,"	CMX:        byte-reverse, shuffle, spectrum reverse, fix floats if necessary\n");
    fprintf(stderr,"	CMXW:       shuffle, spectrum reverse, fix floats if necessary\n");
    fprintf(stderr,"	Spinsight:  shuffle, spectrum reverse, fix floats if necessary\n");
    fprintf(stderr,"	M:          shuffle, spectrum reverse\n");
    fprintf(stderr,"	NMR1:       convert from Vax D_float, shuffle, spectrum reverse\n");
    fprintf(stderr,"	SMIS:       byte-reverse, fix floats\n");
    fprintf(stderr,"If conflicting options are given, the last one wins.\n");
}

int parse_commandline(argc, argv, file1, file2)
int argc;
char * argv[];
int * file1, * file2;
{
    int i;
    int type = OUT_SDATA | OUT_STEXT | MAKE_DIR |
	IN_CMSS | READ_DIR | SHUFFLE | SPECTRUM_REVERSE;
    int retcode = RC_SUCCESS;

    *file1 = 0;
    *file2 = 0;
    exptl_type = 0;
    for (i = 1; i < argc  &&  retcode == 0; i++) {
	if (*argv[i] == '-'  &&  argv[i][1]) {
	    switch (argv[i][1]) {
	    case 'b':
		type &= ~(ALL_IN | ALL_READ | ALL_TRANS);
		type |= IN_BRUKER | READ_FILE | BYTE_REVERSE | SPECTRUM_REVERSE | ALTERNATE_SIGNS;
		if (argv[i][2] == 'b') {
		    type |= BLIND_READ;	/* only place this is recognized */
		    if (i+2 < argc) {	/* must be two more args after this */
			if (sscanf(argv[++i], "%ld", &blind_skip) != 1)
			    retcode = RC_BADNUMBER;
			if (sscanf(argv[++i], "%ld", &blind_length) != 1)
			    retcode = RC_BADNUMBER;
		    }
		    else
			retcode = RC_BADSWITCH;
		}
		break;
	    case 'c':
		type &= ~(ALL_IN | ALL_READ | ALL_TRANS);
		if (argv[i][2] == 's')
		    type |= IN_CMSS | READ_DIR | SHUFFLE | SPECTRUM_REVERSE;
		else if (argv[i][2] == 'w')
		    type |= IN_CMXW | READ_DIR | SHUFFLE | SPECTRUM_REVERSE;
		else
		    type |= IN_CMX  | READ_DIR | SHUFFLE | SPECTRUM_REVERSE | BYTE_REVERSE;
		break;
	    case 'i':
		type &= ~(ALL_IN | ALL_READ | ALL_TRANS);
		type |= IN_SMIS | READ_FILE | BYTE_REVERSE | SPECTRUM_REVERSE;
		break;
	    case 'm':
		type &= ~(ALL_IN | ALL_READ | ALL_TRANS);
		type |= IN_NMR1 | READ_FILE | VAX_FLOAT | SHUFFLE | SPECTRUM_REVERSE;
		break;
	    case 'M':
		type &= ~(ALL_IN | ALL_READ | ALL_TRANS);
		type |= IN_M100 | READ_FILE | SHUFFLE | SPECTRUM_REVERSE;
		break;
	    case 's':
		type &= ~(ALL_IN | ALL_READ | ALL_TRANS);
		type |= IN_SREAD | READ_DIR;
		break;
	    case 'v':
		type &= ~(ALL_IN | ALL_READ | ALL_TRANS);
		type |= IN_VARIAN | READ_DIR;
		break;
	    case 'o':
		type &= ~(ALL_OUT | ALL_MAKE);
		if (argv[i][2] == 'v') {
		    /* Varian Unity is only partially
		       implemented:  we only allow -ovd. */
		    switch (argv[i][3]) {
			/* case '\0':
			   type |= OUT_VFID | OUT_VPARAMS | OUT_VTEXT | MAKE_DIR;
			   break; */
		    case 'd':
			type |= OUT_VFID | MAKE_FILE;
			break;
			/* case 'p':
			   type |= OUT_VPARAMS | MAKE_FILE;
			   break;
			   case 't':
			   type |= OUT_VTEXT | MAKE_FILE;
			   break; */
		    default:
			retcode = RC_BADSWITCH;
			break;
		    }
		}
		else if (argv[i][2] == 's') {
		    switch (argv[i][3]) {
		    case '\0':
			type |= OUT_SDATA | OUT_STEXT | MAKE_DIR;
			break;
		    case 'd':
			type |= OUT_SDATA | MAKE_FILE;
			break;
		    case 'p':
			type |= OUT_STEXT | MAKE_FILE;
			break;
		    default:
			retcode = RC_BADSWITCH;
			break;
		    }
		}
		break;
	    case 'n':
		type &= ~(ALL_OUT | ALL_MAKE);
		type |= NO_HEADER | MAKE_FILE;
		break;
	    case 'a':
		type |= ALTERNATE_SIGNS;
		break;
	    case 'A':
		type &= ~ALTERNATE_SIGNS;
		break;
	    case 'f':
		if (i+1 < argc) {	/* must be one more arg after this */
		    if (argv[i][2] == 'i') {		/* look at next arg */
			if (sscanf(argv[++i], "%lf", &imagfactor) != 1)
			    retcode = RC_BADNUMBER;
			else if (imagfactor == 0)
			    type |= ZERO_IMAGS;
			else
			    type |= SCALE_IMAGS;
		    }
		    else if (argv[i][2] == 'r') {	/* look at next arg */
			if (sscanf(argv[++i], "%lf", &realfactor) != 1)
			    retcode = RC_BADNUMBER;
			else if (realfactor == 0)
			    type |= ZERO_REALS;
			else
			    type |= SCALE_REALS;
		    }
		    else
			retcode = RC_BADSWITCH;
		}
		else
		    retcode = RC_MISSINGARG;
		break;
	    case 'r':
		type |= BYTE_REVERSE;
		break;
	    case 'R':
		type &= ~BYTE_REVERSE;
		break;
	    case '-':
		type |= SPECTRUM_REVERSE;
		break;
	    case '+':
		type &= ~SPECTRUM_REVERSE;
		break;
#ifdef EXPTL
	    case 'x':				/* experimental options */
		switch (argv[i][2]) {
		case 'd':	/* decimate by factor, offset, and method */
		    if (i+3 < argc) {	/* must be 3 more args after this */
			if (sscanf(argv[++i], "%ld", &exptl_p1) != 1)
			    retcode = RC_BADNUMBER;
			else if (sscanf(argv[++i], "%ld", &exptl_p2) != 1)
			    retcode = RC_BADNUMBER;
			else if (sscanf(argv[++i], "%ld", &exptl_p3) != 1)
			    retcode = RC_BADNUMBER;
			else {		/* all the args were found, so OK */
			    type |= EXPERIMENTAL_METHOD;
			    exptl_type |= EXPTLTYPE_DECIMATE;
			}
		    }
		    else
			retcode = RC_MISSINGARG;
		    break;
		default:
		    retcode = RC_BADSWITCH;
		    break;
		}
		break;
#endif /* EXPTL */
	    case '#':
		fprintf(stderr, "%s\n", Version_ID);
		cleanup(RC_SUCCESS);
		break;
	    default:
		retcode = RC_BADSWITCH;
	    }
	}
	else {
	    if (*file1 == 0)
		*file1 = i;
	    else if (*file2 == 0)
		*file2 = i;
	    else
		retcode = RC_BADARG;
	}
    }
    if ((type & ALL_IN == 0)  ||  (type & ALL_OUT == 0)  ||  
	(type &~ALL_MAKE) == 0)
	retcode = RC_NOFORMAT;

    /* Temporary measures since Varian Unity format is only partly supported. */
    if (type & IN_VARIAN) {
	if ((type & MAKE_DIR)  ||
	    (type & OUT_VPARAMS) ||
	    (type & OUT_VTEXT) ||
	    ! (type & OUT_VFID)) {
	    retcode = RC_BADSWITCH;
	}
    }
    if (type & OUT_VFID) {
	if ((type & MAKE_DIR)  ||
	    (type & OUT_VPARAMS) ||
	    (type & OUT_VTEXT) ||
	    ! (type & IN_VARIAN)) {
	    retcode = RC_BADSWITCH;
	}
    }

    if (retcode) {
	usage();
	cleanup(retcode);
    }
    return(type);
}

/* Searches command line for input and output file specs at positions "inarg"
   and "outarg".  If a word is found at position inarg, it is opened as a
   file and the global handle "in" is connected to it.  Otherwise, stdin is
   connected to "in".  The same goes for "out".  If a file cannot be opened,
   exits with an error message.
 */
void open_inout(argc, argv, inarg, outarg, type)
int argc;
char * argv[];
int inarg, outarg;
long type;
{
    FILE * in, * out;

    if (type & READ_FILE) {
	if (inarg > 0  &&  inarg < argc) {
	    in = fopen(argv[inarg], "r");
	    if (in == NULL) {
		errmsg(RC_NOINFILE, __FILE__, __LINE__, (long) argv[inarg]);
		cleanup(RC_NOINFILE);
	    }
	}
	else {
	    in = stdin;
	}
	/* All formats in this category have params and fid in the
	   same file.  We access both through fid_in. */
	fid_in = in;
	params_in = NULL;
    }
    else {
	if (inarg == 0)
	    in_dirname = "convert.in";
	else
	    in_dirname = argv[inarg];
	/* The only formats in this category for which input is fully
	   implemented are CMX, CMXW, and CMSS.  No parameter file is
	   opened for them; they are expected to use open_params_in()
	   to open their parameter files one by one.  For native
	   Varian Unity format, we have a limited implementation
	   ("-vd -ovd") which allows us to open the fid data within
	   the .fid directory, transform the data, and write it out as
	   a plain file. */
	if (type & (IN_CMX | IN_CMXW)) {
	    if ((fid_in = open_file_in_dir(in_dirname, "d", "r")) == NULL) {
		errmsg(RC_NOINFILE, __FILE__, __LINE__, (long) "d");
		cleanup(RC_NOINFILE);
	    }
	}
	else if (type & IN_CMSS) {
	    if ((fid_in = open_file_in_dir(in_dirname, "data", "r")) == NULL) {
		errmsg(RC_NOINFILE, __FILE__, __LINE__, (long) "data");
		cleanup(RC_NOINFILE);
	    }
	}
	else if (type & IN_VARIAN) {
	    if ((fid_in = open_file_in_dir(in_dirname, "fid", "r")) == NULL) {
		errmsg(RC_NOINFILE, __FILE__, __LINE__, (long) "fid");
		cleanup(RC_NOINFILE);
	    }
	}
    }

    if (type & MAKE_FILE) {
	if (outarg > 0  &&  outarg < argc) {
	    out = fopen(argv[outarg], "w");
	    if (out == NULL) {
		errmsg(RC_NOOUTFILE, __FILE__, __LINE__, (long) argv[outarg]);
		cleanup(RC_NOOUTFILE);
	    }
	}
	else
	    out = stdout;
	if (type & (OUT_VFID | OUT_SDATA))
	    fid_out = out;
	else if (type & (OUT_VPARAMS | OUT_STEXT))
	    params_out = out;
	else				/* type & OUT_VTEXT */
	    text_out = out;
    }
    else {				/* need to make a new directory */
	if (outarg == 0)
	    out_dirname = "convert.out";
	else
	    out_dirname = argv[outarg];

	/* The mkdir() call will generate an argument count warning
	   under AmigaDos since the Lattice library function does not
	   use the permission mode argument.  The warning is harmless,
	   though.
	   */
	if (mkdir(out_dirname, 0775) != 0) {
	    errmsg(RC_NOOUTDIR, __FILE__, __LINE__, (long) out_dirname);
	    cleanup(RC_NOOUTDIR);
	}
	if (type & OUT_VFID) {		/* must be Varian dir/file */
	    if ((fid_out = open_file_in_dir(out_dirname, "fid", "w")) == NULL) {
		errmsg(RC_NOOUTFILE, __FILE__, __LINE__, (long) "fid");
		cleanup(RC_NOOUTFILE);
	    }
	    if ((params_out = open_file_in_dir(out_dirname, "procpar", "w")) == NULL) {
		errmsg(RC_NOOUTFILE, __FILE__, __LINE__, (long) "procpar");
		cleanup(RC_NOOUTFILE);
	    }
	    if ((text_out = open_file_in_dir(out_dirname, "text", "w")) == NULL) {
		errmsg(RC_NOOUTFILE, __FILE__, __LINE__, (long) "text");
		cleanup(RC_NOOUTFILE);
	    }
	}
	else {						/* must be sread dir/file */
	    if ((fid_out = open_file_in_dir(out_dirname, "sdata", "w")) == NULL) {
		errmsg(RC_NOOUTFILE, __FILE__, __LINE__, (long) "sdata");
		cleanup(RC_NOOUTFILE);
	    }
	    if ((params_out = open_file_in_dir(out_dirname, "stext", "w")) == NULL) {
		errmsg(RC_NOOUTFILE, __FILE__, __LINE__, (long) "stext");
		cleanup(RC_NOOUTFILE);
	    }
	}
    }
}

/* Open a parameter file in the current input directory and store a FILE
   pointer for it in the place supplied.  If *fpp contains a non-null value,
   it is assumed to be an open FILE pointer and is first closed with fclose().
   If "required" is false and the file does not exist, NULL is returned.  If
   "required" is true and the file does not exist, a fatal error occurs.
*/
void open_params_in(fpp, filename, required)
FILE ** fpp;
char * filename;
int required;
{
    if (*fpp)
	fclose(*fpp);
    *fpp = open_file_in_dir(in_dirname, filename, "r");
    if (required  &&  *fpp == NULL) {
	errmsg(RC_NOINFILE, __FILE__, __LINE__, (long) filename);
	cleanup(RC_NOINFILE);
    }
}

/* Like fopen(), but opens a named file which is contained in a named
   directory.  Handles the construction of the complete pathname as necessary
   for the current operating system.  Does no error checking on fopen()'s
   result so that custom actions can be taken if a NULL pointer is returned.
*/
FILE * open_file_in_dir(dir, file, mode)
char * dir, * file, * mode;
{
    char filename[256];
    int len;
	
    strcpy(filename, dir);
    len = strlen(dir);
    if (filename[len-1] != SLASH) {
	    filename[len] = SLASH;
	    filename[++len] = '\0';
    }
    strcpy(&filename[len], file);
    return(fopen(filename, mode));
}

void errmsg(rc, file, line, extra)
int rc;
char * file;
int line;
long extra;
{
    char * where;

    switch (rc) {
    case RC_NOINFILE:
	fprintf(stderr, "unable to open input file %s (%s:%d)\n",
		(char *) extra, file, line);
	break;
    case RC_NOOUTFILE:
	fprintf(stderr, "unable to open output file %s (%s:%d)\n",
		(char *) extra, file, line);
	break;
    case RC_NOTMPFILE:
	fprintf(stderr, "unable to open temporary file %s (%s:%d)\n",
		(char *) extra, file, line);
	break;
    case RC_NOOUTDIR:
	fprintf(stderr, "unable to create output directory %s (%s:%d)\n",
		(char *) extra, file, line);
	break;
    case RC_NOMEMORY:
	fprintf(stderr, "out of memory, needed %d bytes (%s:%d)\n",
		extra, file, line);
	break;
    case RC_EOF:
	fprintf(stderr, "unexpected end of input file, offset %d (%s:%d)\n",
		extra, file, line);
	break;
    case RC_READERR:
	fprintf(stderr, "unable to read %d bytes from input file (%s:%d)\n",
		extra, file, line);
	break;
    case RC_WRITERR:
	if (extra == 0)
	    where = "file header";
	else if (extra == 1)
	    where = "block header";
	else
	    where = "fid data";
	fprintf(stderr, "unable to write %s to output file (%s:%d)\n",
		where, file, line);
	break;
    case RC_SKIPERR:
	fprintf(stderr, "unable to seek %d bytes in input file (%s:%d)\n",
		extra, file, line);
	break;
    case RC_BADBFLOAT:
	fprintf(stderr, "unrecognized Bruker floating point number %02X %02X %02X %02X %02X %02X (%s:%d)\n",
		((unsigned char *) extra)[0], ((unsigned char *) extra)[1],
		((unsigned char *) extra)[2], ((unsigned char *) extra)[3],
		((unsigned char *) extra)[4], ((unsigned char *) extra)[5],
		file, line);
	break;
    case RC_BADPARAMLINE:
	fprintf(stderr, "cannot interpret '%s' parameter line (%s:%d)\n",
		(char *) extra, file, line);
	break;
    case RC_BADARRAY:
	fprintf(stderr, "inconsistent information for array %s (%s:%d)\n",
		(char *) extra, file, line);
	break;
    case RC_INOTYET:
	fprintf(stderr, "input data type not supported yet (%s:%d)\n",
		file, line);
	break;
    case RC_ONOTYET:
	fprintf(stderr, "output data type not supported yet (%s:%d)\n",
		file, line);
	break;
    case RC_BADDATE:
	fprintf(stderr, "cannot interpret date '%s' (%s:%d)\n",
		(char *) extra, file, line);
	break;
    default:
	fprintf(stderr, "error number %d (%s:%d)\n", rc, file, line);
	break;
    }
}

/* Closes open file pointers if they are not equal to stdin or stdout (which
   are left for the startup module to close since they should not be closed
   twice), releases memory, and exits to the shell with the desired return
   code.
 */
void cleanup(retcode)
int retcode;
{
    if (fid_in != NULL  &&  fid_in != stdin)
	fclose(fid_in);
    if (params_in != NULL  &&  params_in != stdin)
	fclose(params_in);
    if (text_in != NULL  &&  text_in != stdin)
	fclose(text_in);
    if (fid_out != NULL  &&  fid_out != stdout)
	fclose(fid_out);
    if (params_out != NULL  &&  params_out != stdout)
	fclose(params_out);
    if (text_out != NULL  &&  text_out != stdout)
	fclose(text_out);
    if (temp_out != NULL  &&  temp_out != stdout)
	fclose(temp_out);
    if (in_header != NULL)
	free(in_header);
    if (in_data != NULL)
	free(in_data);
    exit(retcode);
}

/* Read the parameter part of the data.  Make any necessary changes in the
   fid type and return the updated type variable.
*/
long read_params(type)
long type;
{
    switch (type & 0xff) {
    case IN_BRUKER:
	bruker_file(type);
	break;
    case IN_M100:
	m100_file(type);
	break;
    case IN_CMX:
    case IN_CMXW:
    case IN_CMSS:
	type = cmx_file(type);
	break;
    case IN_NMR1:
	nmr1_file(type);
	break;
    case IN_VARIAN:
	unity_file(type);
	break;
    case IN_SMIS:
	type = smis_file(type);
	break;
    default:
	fprintf(stderr, "file conversion not implemented yet for type %x\n",
		type);
	cleanup(RC_INOTYET);
    }
    return(type);
}

/* This is intended to become something more than a raw binary read.  For
   example, it should handle Varian multiblock files, which may have block
   headers interspersed with fids.  It might also do something intelligent
   with arrayed datasets, especially the complicated ones a CMX can produce.
   One unusual type is now handled correctly:  Spinsight files, in which
   arrayed fids are stored with the reals noncontiguous with the imaginaries.
   It is possible that the same is true of the IN_CMX and IN_CMXW types, but I
   have no examples to verify this.
*/
void read_fid(type, trace)
long type;
long trace;
{
    int rc;

    if (type & (IN_BRUKER | IN_NMR1 | IN_M100 | IN_CMX | IN_CMXW | IN_VARIAN | IN_SMIS)) {
	rc = read_rawfid(in_fhead.tbytes);
	if (rc != RC_SUCCESS)
	    cleanup(rc);
    }
    else if (type & IN_CMSS) {
	rc = read_rawfid_separated(in_fhead.tbytes,
				   in_fhead.nblocks*in_fhead.ntraces*in_fhead.tbytes/2);
	if (rc != RC_SUCCESS)
	    cleanup(rc);
    }
    else {
	fprintf(stderr, "file conversion not implemented yet for type %x\n",
		type);
	cleanup(RC_INOTYET);
    }
}

/* Read bytes of raw binary from the infile.  A buffer is allocated
   dynamically to hold the data.  If the read is successful, the buffer
   address is placed in the global pointer in_data and RC_SUCCESS is
   returned.  Otherwise, the buffer is freed, in_data is set to NULL, and an
   error code is returned.
*/
int read_rawfid(bytes)
int bytes;
{
	if (in_data) {
		free(in_data);
		in_data = NULL;
	}
	in_data = (unsigned char *) malloc(bytes);
	if (in_data == NULL) {
		errmsg(RC_NOMEMORY, __FILE__, __LINE__, bytes);
		return(RC_NOMEMORY);
	}

	if (fread(in_data, bytes, 1, fid_in) != 1) {
		errmsg(RC_READERR, __FILE__, __LINE__, bytes);
		free(in_data);
		in_data = NULL;
		return(RC_READERR);
	}
	return(RC_SUCCESS);
}

/* Like read_rawfid(), except that half the bytes are read starting at the
   current position in the file, and the other half are read starting at
   "separation" bytes after the current position.  The file is left positioned
   after the end of the first half of data read.  This is for file formats in
   which reals and imaginaries are not shuffled and are stored in
   noncontiguous regions of the file.  For example, Spinsight would store a
   3-fid array like this:  reals1-reals2-reals3-imags1-imags2-imags3.  If the
   file starts out positioned at the beginning, and the args passed are:
      bytes = len(reals1)+len(imags1) and
      separation = len(reals1)+len(reals2)+len(reals3),
   this function will allocate a buffer of length bytes, then read bytes/2
   bytes starting at the beginning of reals1, then another bytes/2 bytes
   starting at the beginning of imags1, leaving the file positioned at the
   start of reals2.

   If "separation" is equal to bytes/2, the two halves of the data are really
   contiguous.  In this case no seeking is done and the behavior is identical
   to read_rawfid().  The same is true for separation < bytes/2, which
   otherwise would specify a bizarre sort of overlap.  Thus
   read_rawfid_separated(bytes,0) and read_rawfid_separated(bytes,bytes/2) are
   both equivalent to read_rawfid(bytes).

   Passing an odd number in "bytes" may cause unexpected results.
*/
int read_rawfid_separated(bytes, separation)
int bytes;
int separation;
{
	int bytes2 = bytes >> 1;	/* half the number of bytes */

	if (in_data) {
		free(in_data);
		in_data = NULL;
	}

	in_data = (unsigned char *) malloc(bytes);
	if (in_data == NULL) {
		errmsg(RC_NOMEMORY, __FILE__, __LINE__, bytes);
		return(RC_NOMEMORY);
	}

	if (fread(in_data, bytes2, 1, fid_in) != 1) {
		errmsg(RC_READERR, __FILE__, __LINE__, bytes2);
		free(in_data);
		in_data = NULL;
		return(RC_READERR);
	}

	if (separation > bytes2) {
		if (fseek(fid_in, separation - bytes2, SEEK_CUR) != 0) {
			errmsg(RC_SKIPERR, __FILE__, __LINE__,
				separation - bytes2);
			cleanup(RC_SKIPERR);
		}
	}

	if (fread(&in_data[bytes2], bytes2, 1, fid_in) != 1) {
		errmsg(RC_READERR, __FILE__, __LINE__, bytes2);
		free(in_data);
		in_data = NULL;
		return(RC_READERR);
	}

	if (separation > bytes2) {
		if (fseek(fid_in, -separation, SEEK_CUR) != 0) {
			errmsg(RC_SKIPERR, __FILE__, __LINE__, 
				separation - bytes2);
			cleanup(RC_SKIPERR);
		}
	}

	return(RC_SUCCESS);
}

/* Perform conversion operations one by one on the data at in_data depending
   on the flags encoded in type, which is not altered by the function.
   in_data itself may be reallocated elsewhere if the data table changes size.
   The order of operations here is important.  The general flow is: change to
   processor's byte order; shuffle reals and imags (r,i,r,i,...); for
   IEEE_FLOATs, scale then fix then zero; for VAX_FLOATs, fix then zero then
   scale; for integers, expand then zero then scale; lastly for all, spectrum
   reverse then alternate signs.  The in_fhead and in_bhead start out being
   the same as out_fhead and out_bhead, except for automatic expansion
   controlled by in_fhead.ebytes < out_fhead.ebytes (e.g. for Bruker 3-byte
   data).  After this automatic expansion, the output headers reflect the
   current state of in_data.  As subsequent transformations are done, the data
   table at in_data may change size and shape, but the output headers will
   always be kept in sync with the data table.  The input headers always will
   remain untouched.
*/
void convert_data(type)
long type;
{
    unsigned char * new_data;
    int i;

    /* do this before most other functions or they will get confused about
       byte order. */
    if (in_fhead.ebytes == 4)
    {
       int *iptr;
       iptr = (int *) in_data;
       for (i=0; i < in_fhead.np; i++)
       {
          int tmp;
          tmp = *iptr;
          *iptr = htonl(tmp);
          iptr++;
       }
    }
    else
    {
       short int *iptr;
       iptr = (short *) in_data;
       for (i=0; i < in_fhead.np; i++)
       {
          short int tmp;
          tmp = *iptr;
          *iptr = htons(tmp);
          iptr++;
       }
    }
    if (type & BYTE_REVERSE)
	reverse_bytes(in_data, in_fhead.np, in_fhead.ebytes);

    /* desegregate reals and imaginaries.  Order of this and expand() is
       arbitrary, but put this first for greater speed. */
    if (type & SHUFFLE) {
	new_data = shuffle(in_data, in_fhead.np, in_fhead.ebytes);
	free(in_data);
	in_data = new_data;
    }

    /* convert IEEE single-precision floating point to integers, but if
       we will be scaling, scale them while they're still floats */
    if (type & IEEE_FLOAT) {
	if (type & SCALE_REALS)
	    scale(in_data, out_fhead.np, out_fhead.ebytes, 0, 1, realfactor);
	if (type & SCALE_IMAGS)
	    scale(in_data, out_fhead.np, out_fhead.ebytes, 1, 1, imagfactor);
	ieee_float(in_data, in_fhead.np);
    }

    /* convert Vax floating point to integers */
    if (type & VAX_FLOAT)
	vax_float(in_data, in_fhead.np);

    /* Automatic expansion of integer data to final size.  After this,
       out_fhead.ebytes and .tbytes are correct for the data, which is still
       called in_data.  in_fhead.np and out_fhead.np were always equal. */
    if (in_fhead.ebytes != out_fhead.ebytes) {
	new_data = expand(in_data, in_fhead.np, in_fhead.ebytes, out_fhead.ebytes);
	free(in_data);
	in_data = new_data;
    }

    if (type & ZERO_REALS)
	zero(in_data, out_fhead.np, out_fhead.ebytes, 0);

    if (type & ZERO_IMAGS)
	zero(in_data, out_fhead.np, out_fhead.ebytes, 1);

    /* scaling for things that didn't start life as IEEE_floats */
    if (!(type & IEEE_FLOAT)) {
	if (type & SCALE_REALS)
	    scale(in_data, out_fhead.np, out_fhead.ebytes, 0, 0, realfactor);
	if (type & SCALE_IMAGS)
	    scale(in_data, out_fhead.np, out_fhead.ebytes, 1, 0, imagfactor);
    }

    if (type & SPECTRUM_REVERSE)
	negate(in_data, out_fhead.np, out_fhead.ebytes, 1);

    if (type & ALTERNATE_SIGNS)
	alternate(in_data, out_fhead.np, out_fhead.ebytes);

#ifdef EXPTL
    if (type & EXPERIMENTAL_METHOD)
	experimental_method(&in_data, out_fhead.np, out_fhead.ebytes, type,
			    exptl_type, exptl_p1, exptl_p2, exptl_p3);
#endif /* EXPTL */
}

/* Convert an array of signed oldsize-byte-ints to signed newsize-byte-ints.
   A new array is allocated and constructed in memory and a pointer to it is
   returned.  The old array is not altered; it will normally be deallocated
   after this function returns.  NON-PORTABLE ASSUMPTION: assumes that the
   most significant byte of the integer is the one with the lowest address and
   that the sign bit is the high bit of that byte.  This is true on
   "big-endian" CPUs like the 680x0, SPARC, and Aspect.  On "little-endian"
   machines like the 80x86 and VAX, this will have to be changed!
*/
unsigned char * expand(old_data, elements, oldsize, newsize)
unsigned char * old_data;
int oldsize, newsize;
int elements;
{
    unsigned char * new_data;
    int i, j;
    unsigned char * oldp, * newp;
    unsigned char fill;
    int e;

    if (oldsize > newsize) {
	fprintf(stderr, "internal error:  cannot contract data size\n");
	cleanup(RC_INTERNAL);
    }

    /* allocate a new array to replace old one */
    new_data = (unsigned char *) malloc(elements * newsize);
    if (new_data == NULL) {
	errmsg(RC_NOMEMORY, __FILE__, __LINE__, elements * newsize);
	cleanup(RC_NOMEMORY);
    }

    if (oldsize == newsize) {		/* if same size, just copy */
	memcpy(new_data, old_data, elements * newsize);
    }
    else {				/* newsize must be > oldsize */
	/* oldp and newp are pointers to 1st byte of element e in each array */
	oldp = old_data;
	newp = new_data;
	for (e = 0; e < elements; e++) {		/* for each element */
	    for (i = oldsize-1, j = newsize-1; i >= 0; i--, j--)
		newp[j] = oldp[i];	/* copy bytes from low to high */
	    /* i goes to 0, j left as number of filler bytes still needed */

	    if (*oldp & 0x80)			/* negative sign */
		fill = 0xFF;			/* need sign extension */
	    else
		fill = 0;				/* fill with 0's */

	    for (; j >= 0; j--)			/* fill remaining bytes */
		newp[j] = fill;

	    oldp += oldsize;			/* next element */
	    newp += newsize;
	}
    }
    return(new_data);
}

/* Reverse the order of bytes within each word of an array.  Generally this
   is done to convert what was once an array of integers on an 80x86 or VAX
   or Aspect to an array of integers (the same ones but stored differently)
   usable by a 680x0 or SPARC.  The old data array is overwritten by the new
   one.
*/
void reverse_bytes(old_data, elements, size)
unsigned char * old_data;
int elements;
int size;
{
	unsigned char * oldp;
	int i;
	int size_1;
	unsigned char temp;
	int size_2;
	int e;

	size_2 = size / 2;		/* rounds down, so =1 when reversing 3-byte ints */
	size_1 = size - 1;		/* highest array index */

	/* oldp points to 1st byte of element e in array */
	for (oldp = old_data, e = 0; e < elements; e++, oldp += size) {
		for (i = 0; i < size_2; i++) {
			temp = oldp[i];
			oldp[i] = oldp[size_1-i];
			oldp[size_1-i] = temp;
		}
	}
}

/* Shuffle data so that each real point is followed by its imaginary, rather
   than having all reals together followed by all imaginaries.  A new array
   is allocated and constructed; the old data is not touched.  This is needed
   for data from GE/Nicolet and CMX spectrometers.
*/
unsigned char * shuffle(old_data, elements, size)
unsigned char * old_data;
int elements;
int size;
{
	unsigned char * new_data;
	unsigned char * newp, * realp, * imagp;
	int i;
	int e, elements_2;

	/* allocate a new array to replace old one */
	new_data = (unsigned char *) malloc(elements * size);
	if (new_data == NULL) {
		errmsg(RC_NOMEMORY, __FILE__, __LINE__, elements * size);
		cleanup(RC_NOMEMORY);
	}

	elements_2 = elements >> 1;

	/* realp, imagp are pointers to 1st byte of real and imaginary element
	   e in each array */
	realp = old_data;
	imagp = &old_data[elements*size/2];
	newp = new_data;
	for (e = 0; e < elements_2; e++) {
		for (i = 0; i < size; i++) {
			newp[i] = realp[i];
			newp[i+size] = imagp[i];
		}
		realp += size;
		imagp += size;
		newp += size+size;
	}
	return(new_data);
}

/* Negate either all the reals or all the imaginaries, depending on whether
   the value of offset is 0 or 1.  For speed and convenience, implemented
   using hardware math:  three separate loops for longword, shortword, and
   byte-sized integers.  (I don't ever expect to have to run this on an
   Aspect.)
*/
void negate(old_data, elements, size, offset)
unsigned char * old_data;
int elements;
int size;
int offset;
{
	unsigned char * oldp;
	short shortword;
	int e, elements_2;
	SIGNEDCHAR byteword;

	/* oldp points to 1st byte of element e in array */
	oldp = old_data + size*offset;
	elements_2 = elements >> 1;
	switch (size) {
	case 4:
                {
                   int *iptr;
                   iptr = (int *) oldp;
		for (e = 0; e < elements_2; e++) {
                   *iptr = - *iptr;
                    iptr += 2;
/*
			longword = *((long *) oldp);
			longword = -longword;
			*((long *) oldp) = longword;
			oldp += 8;
 */
		}
                }
		break;
	case 2:
		for (e = 0; e < elements_2; e++) {
			shortword = *((short *) oldp);
			shortword = -shortword;
			*((short *) oldp) = shortword;
			oldp += 4;
		}
		break;
	case 1:
		for (e = 0; e < elements_2; e++) {
			byteword = *((SIGNEDCHAR *) oldp);
			byteword = -byteword;
			*((SIGNEDCHAR *) oldp) = byteword;
			oldp += 2;
		}
		break;
	default:
		fprintf(stderr, "can only do math on 1-, 2-, and 4-byte words\n");
		cleanup(RC_NOMATH);
	}
}

/* Negate every 2nd complex point; that is, multiply the data array by
   1,1,-1,-1,1,1,-1,-1,...  For speed and convenience, implemented
   using hardware math:  three separate loops for longword, shortword, and
   byte-sized integers.  (I don't ever expect to have to run this on an
   Aspect.)
*/
void alternate(old_data, elements, size)
unsigned char * old_data;
int elements;
int size;
{
	unsigned char * oldp;
	long longword;
	short shortword;
	int e, elements_4;
	SIGNEDCHAR byteword;

	/* oldp points to 1st byte of 2nd real element in array */
	oldp = old_data + 2*size;
	elements_4 = elements >> 2;
	switch (size) {
	case 4:
		for (e = 0; e < elements_4; e++) {
			longword = *((long *) oldp);
			longword = -longword;
			*((long *) oldp) = longword;
			oldp += 4;			/* now do imag */
			longword = *((long *) oldp);
			longword = -longword;
			*((long *) oldp) = longword;
			oldp += 12;			/* skip next pair */
		}
		break;
	case 2:
		for (e = 0; e < elements_4; e++) {
			shortword = *((short *) oldp);
			shortword = -shortword;
			*((short *) oldp) = shortword;
			oldp += 2;			/* now do imag */
			shortword = *((short *) oldp);
			shortword = -shortword;
			*((short *) oldp) = shortword;
			oldp += 6;			/* skip next pair */
		}
		break;
	case 1:
		for (e = 0; e < elements_4; e++) {
			byteword = *((SIGNEDCHAR *) oldp);
			byteword = -byteword;
			*((SIGNEDCHAR *) oldp) = byteword;
			oldp += 1;			/* now do imag */
			byteword = *((SIGNEDCHAR *) oldp);
			byteword = -byteword;
			*((SIGNEDCHAR *) oldp) = byteword;
			oldp += 3;			/* skip next pair */
		}
		break;
	default:
		fprintf(stderr, "can only do math on 1-, 2-, and 4-byte words\n");
		cleanup(RC_NOMATH);
	}
}

/* Zeroes the real or imaginary data, depending on whether offset is 0 or 1. 
   Done on a byte-wise basis for generality.
*/
void zero(old_data, elements, size, offset)
unsigned char * old_data;
int elements;
int size;
{
	unsigned char * oldp;
	int i, e, elements_2;

	/* oldp points to 1st byte of element e in array */
	oldp = old_data + size*offset;
	elements_2 = elements >> 1;
	for (e = 0; e < elements_2; e++) {
		for (i = 0; i < size; i++)
			oldp[i] = '\0';
		oldp += size+size;
	}
}

/* Convert Vax floating point (D_floating) to long integers.  For now, no
   scaling is done, but the fraction is rounded.  The conversion is done
   in place.
*/
void vax_float(old_data, elements)
unsigned char * old_data;
int elements;
{
	unsigned char c;
	unsigned char * oldp;
	int e;

	/* oldp points to 1st byte of element e in array */
	oldp = old_data;
	for (e = 0; e < elements; e++) {
		c = oldp[0];			/* reverse bytes at shortword level */
		oldp[0] = oldp[1];
		oldp[1] = c;
		c = oldp[2];
		oldp[2] = oldp[3];
		oldp[3] = c;

		* ((long *) oldp) = (long) (* ((float *) oldp) + 0.5);	/* to integer*/
		* ((long *) oldp) >>= 2;				/* divide by 4 */
		oldp += 4;
	}
}

/* Convert IEEE single-precision (32-bit) floating point to long integers.
   For now, no scaling is done, but the fraction is rounded.  (The CMX
   floating point fids I have seen tend to have numbers ranging from 0 to +/-
   a few thousand but with zero fractional part.  SMIS fids have similar
   magnitudes but fractions are nonzero.)  The conversion is done in place.
*/
void ieee_float(old_data, elements)
unsigned char * old_data;
int elements;
{
	unsigned char * oldp;
	int e;

	/* oldp points to 1st byte of element e in array */
	oldp = old_data;
	for (e = 0; e < elements; e++) {
		* ((long *) oldp) = (long) (* ((float *) oldp) + 0.5);	/* to integer*/
		oldp += 4;
	}
}

/* Scales the real or imaginary data, depending on whether offset is 0 or 1. 
   Scaling is done in double precision floating point, which provides
   generality and precision but not speed.  If is_float is 1 and size is 4,
   data is assumed to be 4-byte IEEE floating point.  If size is 4 and
   is_float is 0, or if size is 1 or 2, data is assumed to be integers.  We
   still need to code four different loops depending on what kind of
   conversion is necessary to go to and from double.  For specialized
   operations like factors of 2, there may be a better way.
*/
void scale(old_data, elements, size, offset, is_float, factor)
unsigned char * old_data;
int elements;
int size;
int offset;
int is_float;
double factor;
{
	unsigned char * oldp;
	int e, elements_2;
	double dbl;

	/* oldp points to 1st byte of element e in array */
	oldp = old_data + size*offset;
	elements_2 = elements >> 1;
	switch (size) {
	case 4:
		if (is_float) {
			for (e = 0; e < elements_2; e++) {
				dbl = *((float *) oldp);
				dbl *= factor;
				*((float *) oldp) = dbl;
				oldp += 8;
			}
		}
		else {
			for (e = 0; e < elements_2; e++) {
				dbl = *((long *) oldp);
				dbl *= factor;
				*((long *) oldp) = dbl;
				oldp += 8;
			}
		}
		break;
	case 2:
		for (e = 0; e < elements_2; e++) {
			dbl = *((short *) oldp);
			dbl *= factor;
			*((short *) oldp) = dbl;
			oldp += 4;
		}
		break;
	case 1:
		for (e = 0; e < elements_2; e++) {
			dbl = *((SIGNEDCHAR *) oldp);
			dbl *= factor;
			*((SIGNEDCHAR *) oldp) = dbl;
			oldp += 2;
		}
		break;
	default:
		fprintf(stderr, "can only do math on 1-, 2-, and 4-byte words\n");
		cleanup(RC_NOMATH);
	}
}

void write_data(type)
long type;
{
        extern int hyper;
        int extraLen;
	if (!(type & NO_HEADER)) {
		write_fhead(&out_fhead);
		write_bhead(&out_bhead);
	}
	write_rawfid(out_fhead.tbytes);
        extraLen = (((out_fhead.tbytes + 511) / 512) * 512) - out_fhead.tbytes;
        if (extraLen)
        {
           write_rawfid(extraLen);
        }
        if (hyper)
        {
	   if (!(type & NO_HEADER)) {
		write_fhead(&out_fhead);
		write_bhead(&out_bhead);
	   }
	   negate(in_data+out_fhead.tbytes, out_fhead.np, out_fhead.ebytes, 1);
	   if (fwrite((in_data+out_fhead.tbytes), out_fhead.tbytes, 1, fid_out) != 1) {
		errmsg(RC_WRITERR, __FILE__, __LINE__, 2);
		cleanup(RC_WRITERR);
	   }
           if (extraLen)
           {
              write_rawfid(extraLen);
           }
        }
}

void write_fhead(head)
struct datafilehead * head;
{
	if (fwrite((char *) head, sizeof(struct datafilehead), 1, fid_out) == 0) {
		errmsg(RC_WRITERR, __FILE__, __LINE__, 0);
		cleanup(RC_WRITERR);
	}
}

void write_bhead(head)
struct datablockhead * head;
{
	if (fwrite((char *) head, sizeof(struct datablockhead), 1, fid_out) == 0) {
		errmsg(RC_WRITERR, __FILE__, __LINE__, 1);
		cleanup(RC_WRITERR);
	}
}

void write_rawfid(bytes)
int bytes;
{
	if (fwrite(in_data, bytes, 1, fid_out) != 1) {
		errmsg(RC_WRITERR, __FILE__, __LINE__, 2);
		cleanup(RC_WRITERR);
	}
}
