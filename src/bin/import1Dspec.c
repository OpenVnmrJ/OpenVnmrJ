/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* import1Dspec - create phasefile from ASCII spectrum */

/*
 Syntax:      import1Dspec <-debug> <-fn ##> <-vs #.#> ascii_file \
			   <phasefile <data>>
	      import1Dspec -v
	      

 Description: "import1Dspec" creates a 1D VnmrJ / VNMR phasefile from an ASCII
	      spectrum. The resulting phasefile can be imported into VNMR /
              VnmrJ using the macro "import1Dspec".

 Arguments:   "ascii_file": path to a pure ASCII file with either Y data (such
	      as from "writetrace") or X,Y data (such as from "writexy").
	      "source/import1Dspec.c" is a C program that can be compiled with
               		cc -O -o /vnmr/bin/import1Dspec import1Dspec.c -m32
	      or (for a local installation)
               		cc -O -o ~/bin/import1Dspec import1Dspec.c -m32

	      "phasefile": Optional path to a binary "phasefile" that can
	      afterwards be imported into VNMR / VnmrJ using "import1Dspec";
	      the default output file uses the same name as the ASCII file,
	      but with ".phf" extension.

	      "data": Optional path to a binary "data" file that is required
 	      when importing "phasefile";  the default data file uses the same
	      name as the phasefile, but with ".dat" extension.

	      "-fn ##": Optional, creates a phasefile with the specified
	      number of points (fn/2 in VNMR!!!); should NOT be necessary,
	      unless the ASCII file is somehow truncated; by default,
	      "import1Dspec" will "zerofill" (add flat baseline at the
	      high-field end) if the ASCII file does not contain a power of 2
	      in points; the argument following "-fn" MUST be numeric; if the
	      specified number is NOT a power of 2, it will be rounded UP to
	      the next higher power of 2. If the specified number or its next
	      higher power of 2 are smaller than the number of points in the
	      ASCII file, the spectrum is truncated at the high-field end.

	      "-vs ##": Optional, permits specifying a (down)scaling factor.
	      When writing spectra in "ai" (absolute intensity) mode,
	      "writetrace" writes out Y values in mm (spectrum multiplied by
	      "vs"); specifying "-vs" with the value of "vs" from VNMR permits
	      recreating the original ("ai") spectrum. Also here, the argument
	      following "-vs" MUST be numeric and positive. The default "vs"
	      value (downscaling factor) is 1.0.

	      "-debug" switches on verbose mode - for debugging purposes ONLY.

	      "-v" reports C code version and date, then exits.

 Examples:    import1Dspec spectrum.txt
	      import1Dspec spectrum.xy
	      import1Dspec spectrum.txt phasefile
	      import1Dspec spectrum.txt phasefile data
              import1Dspec spectrum.xy phasefile data
              import1Dspec -fn 64000 spectrum.xy datdir/phasefile datdir/data
              import1Dspec -vs 327.54 spectrum.xy datdir/phasefile datdir/data
              import1Dspec -vs 327.54 -fn 32000 spectrum.xy

 Related: writetrace - Create ascii file from phasefile (f1 or f2) trace (M)
          writexy - Create x,y ascii file from phasefile (f1 or f2) trace (M)

 Revision history:
   2004-10-29 - r.kyburz, first version
   2006-02-20 - r.kyburz, adjusted for Linux / MacOS X; enhanced argument
		checking; fixed file naming bug
   2006-02-24 - r.kyburz, fixed bug with byte swapping
   2006-03-15 - r.kyburz, fixed byte swapping for Intel-based Macs (courtesy
                Pascal Mercier, Chenomx)
   2008-07-08 - Enhanced PC/Linux architecture detection

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>


static char rev[] =     "import1Dspec.c 3.7";
static char revdate[] = "2009-03-17_09:34";

static char cmd[] = "import1Dspec";
static int debug = 0;
static int swap = 0;

#define FNMIN 32
#define MAX   256

#define S_DATA		0x1		/* 0 = no data, 1 = data */
#define S_SPEC		0x2		/* 0 = FID, 1 = spectrum */
#define S_32		0x4		/* 0 = 16-bit, 1 = 32-bit */
#define S_FLOAT		0x8		/* 0 = integer, 1 = floating point */
#define S_COMPLEX	0x10		/* 0 = real, 1 = complex */
#define S_HYPERCOMPLEX	0x20		/* 1 = hypercomplex */

#define S_ACQPAR	0x80		/* 0 = not Acqpar, 1 = Acqpar */
#define S_SECND 	0x100		/* 0 = first FT, 1 = second FT */
#define S_TRANSF	0x200		/* 0 = regular, 1 = transposed */
#define S_NP    	0x800		/* 1 = np dimension is active */
#define S_NF    	0x1000		/* 1 = nf dimension is active */
#define S_NI    	0x2000		/* 1 = ni dimension is active */
#define S_NI2    	0x4000		/* 1 = ni2 dimension is active */

#define MORE_BLOCKS	0x80		/* 0 = absent, 1 = present */
#define NP_CMPLX	0x100		/* 0 = real, 1 = complex */
#define NF_CMPLX	0x200		/* 0 = real, 1 = complex */
#define NI_CMPLX	0x400		/* 0 = real, 1 = complex */
#define NI2_CMPLX	0x800		/* 0 = real, 1 = complex */

/*--------------------+
| defining structures |
+--------------------*/
struct fileHeader
{
  int nblocks, ntraces, np, ebytes, tbytes, bbytes;
  short vers_id, status;
  int nbheaders;
};
struct blockHeader
{
  short scale, status, index, mode;
  int ctcount;
  float lpval, rpval, lvl, tlt;
};

static int nrecords, ifn, fn = 0;
static double vs = 1.0;
FILE *ascii, *phfil, *data;
int ascii_arg=1;


/*-----------------------------------------------------------------+
| swapbytes - Byte swapping for big-endian vs. little-endian issue |
+-----------------------------------------------------------------*/

void swapbytes(void *data, int size, int num)
{
  unsigned char *c, cs;
  int i, j;
  c = (unsigned char *) data;
  for (j = 0; j < num; j++)
  {
    for (i = 0; i < size/2; i++)
    {
      cs = c[size * j + i];
      c[size * j + i] = c[size * j + (size - (i + 1))];
      c[size * j + (size - (i + 1))] = cs;
    }
  }
}


/*-------------------------------------------------------+
| abortrecipe - abort with a recipe and an error message |
+-------------------------------------------------------*/

void abortrecipe(cmd, msg)
char *cmd, *msg;
{
  (void) fprintf(stderr,
  	"Usage:  %s <-fn #> <-vs #.#> ascii_file <phasefile <data>>\n",
        cmd);
  if (msg[0] != '\0')
  {
    (void) fprintf(stderr, "        %s\n", msg);
  }
  (void) fprintf(stderr, "        command aborted.\n\n");
  exit(1);
}



/*--------------+
| ============= |
| MAIN FUNCTION |
| ============= |
+--------------*/

main (argc, argv)
  int argc;
  char *argv[];
{

  /*------------+
  | Definitions |
  +------------*/

  /* defining variables */
  struct fileHeader fheader;
  struct blockHeader bheader;
  int asciiarg = 1, phfarg = 2;
  char *asciiname, ext[MAX], phfname[MAX], datname[MAX], line[MAX];
  int ch, lastch, ok, i, j, xy = 0, len, res;
  float *spectrum, *cspectrum, val, dummy;
  char cmd[MAX], arch[64];
  struct utsname *s_uname;


  len = strlen(argv[0]);
  i = len;
  while ((argv[0][i-1] != '/') && (i > 0))
  {
    i--;
  }
  j = 0;
  for (; i < len; i++)
  {
    cmd[j] = argv[0][i];
    j++;
  }
  cmd[j] = '\0';


  /*-------------------+
  | checking arguments |
  +-------------------*/

  /* number of arguments: 1 at least */
  if (argc < 2)
  {
    abortrecipe(cmd, "Not enough arguments.");
  }

  i = 1;
  while ((i < argc) && (argv[i][0] == '-'))
  {

    /* -v / -version option prints version and exits, ignoring other args */
    if ((!strcasecmp(argv[i], "-v")) || (!strcasecmp(argv[i], "-version")))
    {
      (void) printf("%s (%s)\n", rev, revdate);
      exit(0);
    }

    if ((!strcasecmp(argv[i], "-d")) || (!strcasecmp(argv[i], "-debug")))
    {
      debug = 1;
    }
      
    else if (!strcasecmp(argv[i], "-fn"))
    {
      i++;
      if (argc < i + 2)
      {
        abortrecipe(cmd, "(1) Argument(s) missing");
      }

      if (sscanf(argv[i], "%ld", &fn) != 1)
      {
	abortrecipe(cmd, "argument following '-fn' must be an integer!");
      }

      if (fn <= 0)
      {
	abortrecipe(cmd, "argument following '-fn' must be an integer and >0");
      }

      if (debug)
      {
	(void) printf("FN set to %d\n", fn);
      }
    }

    else if (!strcasecmp(argv[i], "-vs"))
    {
      i++;
      if (argc < i + 2)
      {
        abortrecipe(cmd, "(2) Argument(s) missing");
      }

      if (sscanf(argv[i], "%lf", &vs) != 1)
      {
        abortrecipe(cmd, "argument following '-vs' must be numeric!");
      }

      if (vs <= 0.0)
      {
        abortrecipe(cmd, "argument following '-vs' must be numeric and >0.0!");
      }

      if (debug)
      {
	(void) printf("VS set to %f\n", vs);
      }
    }

    i++;
  }

  asciiarg = i;
  phfarg = i + 1;

  /* number of arguments, options included */
  if (argc <= asciiarg)
  {
    abortrecipe(cmd, "File name argument(s) missing");
  }
  if (argc > (phfarg + 2))
  {
    abortrecipe(cmd, "Too many arguments");
  }


  /*------------------------------------------------------+
  | check current system architecture (for byte swapping) |
  +------------------------------------------------------*/

  s_uname = (struct utsname *) malloc(sizeof(struct utsname));
  ok = uname(s_uname);
  if (ok >= 0)
  {
    if (debug)
    {
      (void) fprintf(stderr, "\nExtracted \"uname\" information:\n");
      (void) fprintf(stderr, "   s_uname->sysname:   %s\n", s_uname->sysname);
      (void) fprintf(stderr, "   s_uname->nodename:  %s\n", s_uname->nodename);
      (void) fprintf(stderr, "   s_uname->release:   %s\n", s_uname->release);
      (void) fprintf(stderr, "   s_uname->version:   %s\n", s_uname->version);
      (void) fprintf(stderr, "   s_uname->machine:   %s\n", s_uname->machine);
    }

    /* PC / Linux or Mac / Intel / MacOS X marchitecture */
    if ((char *) strstr(s_uname->machine, "86") != (char *) NULL)
    {
      if (debug)
      {
        (void) fprintf(stderr, "   Intel x86 architecture:");
      }
      swap = 1;
    }

    /* Sun / SPARC architecture */
    else if (!strncasecmp(s_uname->machine, "sun", 3))
    {
      if (debug)
      {
        (void) fprintf(stderr,
                "   \"%s\" (Sun SPARC) architecture:", s_uname->machine);
      }
      swap = 0;
    }

    /* PowerMac / MacOS X */
    else if (!strncasecmp(s_uname->machine, "power macintosh", 15))
    {
      if (debug)
      {
        (void) printf("   \"%s\" architecture (PowerPC / MacOS X):",
                s_uname->machine);
      }
      swap = 0;
    }

    /* OTHER ARCHITECTURES */
    else
    {
      if (debug)
      {
        (void) fprintf(stderr, "   \"%s\" architecture:", s_uname->machine);
      }
      swap = 1;
    }

    if (debug)
    {
      if (swap)
      {
        (void) fprintf(stderr, "  SWAPPING BYTES on Varian data\n");
      }
      else
      {
        (void) fprintf(stderr, "  NOT swapping bytes on Varian data\n");
      }
    }
  }
  else
  {
    (void) fprintf(stderr,
        "%s:  unable to determine system architecture, aborting.\n", cmd);
    exit(1);
  }


  /*--------------------------------+
  | first file name arg: ASCII file |
  +--------------------------------*/
  asciiname = argv[asciiarg];


  /*--------------------------------+
  | second file name arg: phasefile |
  +--------------------------------*/
  if (argc > phfarg + 1)
  {
    (void) strcpy(phfname, argv[phfarg]);
    (void) strcpy(datname, argv[phfarg + 1]);
  }
  else if (argc > phfarg)
  {
    (void) strcpy(phfname, argv[phfarg]);
    (void) strcpy(datname, phfname);
    len = strlen(datname);
    j = 0;
    for (i = len-4; i <= len; i++)
    {
      if (i > 0)
      {
        ext[j] = datname[i];
        j++;
      }
    }
    if (!strcasecmp(ext, ".phf"))
    {
      (void) strncpy(datname, phfname, len - 4);
      datname[len - 4] = '\0';
    }
    (void) strcat(datname, ".dat");
  }
  else
  {

    /*---------------------------------------------------------------------+
    | No phasefile name specified - construct name from name of ASCII file |
    +---------------------------------------------------------------------*/
    len = strlen(asciiname);
    j = 0;
    for (i = len-4; i <= len; i++)
    {
      if (i > 0)
      {
        ext[j] = asciiname[i];
        j++;
      }
    }
    if ((!strcmp(ext, ".txt")) || (!strcmp(ext, ".TXT")))
    {
      (void) strncpy(phfname, asciiname, len - 4);
      phfname[len - 4] = '\0';
    }
    else
    {
      j = 0;
      for (i = len-3; i <= len; i++)
      {
        if (i > 0)
        {
          ext[j] = asciiname[i];
          j++;
        }
      }
      if ((!strcmp(ext, ".xy")) || (!strcmp(ext, ".XY")))
      {
        (void) strncpy(phfname, asciiname, len - 3);
        phfname[len - 3] = '\0';
      }
      else
      {
        (void) strcpy(phfname, asciiname);
      }
      (void) strcpy(datname, phfname);
    }

    /*---------------------------------------------------------+
    | Add extension ".phf" to constructed filename (phasefile) |
    | Add extension ".dat" to constructed filename (data)      |
    +---------------------------------------------------------*/
    strcat(phfname, ".phf");
    strcat(datname, ".dat");
  }

  if (debug)
  {
    (void) printf("ASCII file:      %s\n", asciiname);
    (void) printf("phasefile file:  %s\n", phfname);
    (void) printf("data file:       %s\n", datname);
  }


  /*---------------------------------------------------+
  | if "fn" was specified, make sure it's a power of 2 |
  +---------------------------------------------------*/
  if (fn != 0)
  {
    ifn = fn;
    fn = FNMIN;
    while (fn < ifn)
    {
      fn *= 2;
    }
    if ((ifn != fn) || (debug))
    {
      if (ifn < FNMIN)
        (void) printf("\"fn\" set to minimum value of %ld\n", fn);
      else
        (void) printf("\"fn\" set to      %ld\n", fn);
    }
  }


  /*------------------------+
  | check ASCII file access |
  +------------------------*/
  /* open ascii file and read file header */
  ascii = fopen(asciiname, "r");
  if (ascii == NULL)
  {
    (void) fprintf(stderr, "%s:  problem opening file %s\n", cmd, asciiname);
    exit(1);
  }


  /*----------------------------------------+
  | read first line and determine data type |
  +----------------------------------------*/
  i = 0;
  lastch = '\0';
  line[i] = '\0';
  while (((line[i] = (char) fgetc(ascii)) != '\n') && (i < MAX))
  {
    i++;
  }
  line[i] = '\0';
  if (debug)
  {
    (void) printf("line read from input file:\n");
    (void) printf("        %s\n", line);
  }
  i = 0;
  while (((line[i] == ' ') || (line[i] == '\t')) && (line[i] != '\0'))
  {
    if (debug)
    {
      (void) printf("skipping white space ...\n");
    }
    i++;
  }
  while ((((line[i] >= '0') && (line[i] <= '9')) || (line[i] == '-') ||
          (line[i] == '.') || (line[i] == 'e') || (line[i] == 'E')) &&
         (line[i] != '\0'))
  {
    if (debug)
    {
      (void) printf("skipping first number on line ...\n");
    }
    i++;
  }
  while (((line[i] == ',') || (line[i] == ' ') || (line[i] == '\t')) &&
         (line[i] != '\0'))
  {
    if (debug)
    {
      (void) printf("skipping white space ...\n");
    }
    i++;
  }
  while ((line[i] != '\0') && (xy == 0))
  {
    if ((line[i] >= '0') && (line[i] <= '9') && (xy == 0))
    {
      if (debug)
      {
	(void) printf("second number found on line, X,Y data.\n");
      }
      xy = 1;
    }
    i++;
  }
  if (debug)
  {
    if (xy)
      (void) printf("X,Y .. X,Y data\n");
    else
      (void) printf("Y .. Y data\n");
  }


  /*-------------------------------------------------+
  | evaluate number of non-empty lines in input file |
  +-------------------------------------------------*/
  (void) fseek(ascii, 0, 0);
  nrecords = 0;
  lastch = '\0';
  ch = fgetc(ascii);
  while ((ch != '\0') && (ch != EOF))
  {
    if ((ch == '\n') && (lastch != '\n') && (lastch != '\0'))
      nrecords++;
    lastch = ch;
    ch = fgetc(ascii);
  }
  if (lastch != '\n')
    nrecords++;
  if (debug)
  {
    (void) printf("number of records in input file:  %ld\n", nrecords);
  }
  if (fn == 0)
  {
    ifn = nrecords;
    nrecords = FNMIN;
    while (nrecords < ifn)
    {
      nrecords *= 2;
    }
    if (debug)
    {
      if (ifn < FNMIN)
        (void) printf("nrecords set to minimum value of %ld\n", nrecords);
      else
        (void) printf("nrecords set to  %ld\n", nrecords);
    }
  }
  else
    nrecords = fn;


  /*--------------------------------+
  | allocate memory for binary data |
  +--------------------------------*/
  spectrum = (float *) calloc(nrecords, sizeof(float));
  if (spectrum == NULL)
  {
    (void) fprintf(stderr,
	"import1Dspec: allocating memory for phasefile failed\n");
    (void) fclose(ascii);
    exit(1);
  }
  cspectrum = (float *) calloc(nrecords*2, sizeof(float));
  if (cspectrum == NULL)
  {
    (void) fprintf(stderr,
	"import1Dspec: allocating memory for complex data failed\n");
    (void) fclose(ascii);
    exit(1);
  }


  /*================+
  | READ ASCII FILE |
  +================*/
  fseek(ascii, 0, 0);
  i = 0;
  res = 1;
  while ((i < nrecords) && (res != 0))
  {
    if (xy)
    {
      res = fscanf(ascii, "%g %g\n", &dummy, &val);
      if (res != 2)
	res = 0;
    }
    else
    {
      res = fscanf(ascii, "%g\n", &val);
    }
    if (res != 0)
    {
      spectrum[i] = val/vs;
      i++;
    }
  }
  (void) fclose(ascii);


  /*--------------------------------------------------+
  | set "fn" from number of records, if not specified |
  +--------------------------------------------------*/
  if (fn == 0)
  {
    fn = FNMIN;
    while (fn < i)
    {
      fn *= 2;
    }
    if (debug)
    {
      if (ifn < FNMIN)
        (void) printf("\"fn\" set to minimum value of %ld,", fn);
      else
        (void) printf("\"fn\" set to      %ld,", fn);
      (void) printf(" based on # of successful reads\n");
    }
  }


  /*--------------------------------+
  | construct phasefile file header |
  +--------------------------------*/
  fheader.nblocks   = (int)   1;
  fheader.ntraces   = (int)   1;
  fheader.np        = (int)   fn;
  fheader.ebytes    = (int)   4;
  fheader.tbytes    = (int)   (fheader.np * fheader.ebytes);
  fheader.bbytes    = (int)   (fheader.tbytes*fheader.ntraces +
			       sizeof(bheader));
  fheader.vers_id   = (short) 0xc1;	/* (taken from example) */
  fheader.status    = (short) (S_DATA + S_SPEC + S_FLOAT + S_NP);
  fheader.nbheaders = (int)   1;


  /*---------------------------------+
  | construct phasefile block header |
  +---------------------------------*/
  bheader.scale   = (short) 0;
  bheader.status  = (short) (S_DATA + S_SPEC + S_FLOAT);
  bheader.index   = (short) 0;
  bheader.mode    = (short) S_DATA;
  bheader.ctcount = (int)   0;
  bheader.lpval   = (float) 0.0;
  bheader.rpval   = (float) 0.0;
  bheader.lvl     = (float) 0.0;
  bheader.tlt     = (float) 0.0;



  /*----------------------+
  | try to open phasefile |
  +----------------------*/
  phfil = fopen(phfname, "w");
  if (phfil == NULL)
  {
    (void) fprintf(stderr, "import1Dspec:  problem opening output file %s\n",
		   phfname);
    (void) fclose(ascii);
    exit(1);
  }

  /*------------------------------------+
  | swap bytes in headers, if necessary |
  +------------------------------------*/
  if (swap)
  {
    if (debug)
    {
      (void) printf("Swapping bytes in headers for writing\n");
    }
    swapbytes(&fheader.nblocks,   sizeof(int),   1);
    swapbytes(&fheader.ntraces,   sizeof(int),   1);
    swapbytes(&fheader.np,        sizeof(int),   1);
    swapbytes(&fheader.ebytes,    sizeof(int),   1);
    swapbytes(&fheader.tbytes,    sizeof(int),   1);
    swapbytes(&fheader.bbytes,    sizeof(int),   1);
    swapbytes(&fheader.vers_id,   sizeof(short), 1);
    swapbytes(&fheader.status,    sizeof(short), 1);
    swapbytes(&fheader.nbheaders, sizeof(int),   1);

    swapbytes(&bheader.scale,     sizeof(short), 1);
    swapbytes(&bheader.status,    sizeof(short), 1);
    swapbytes(&bheader.index,     sizeof(short), 1);
    swapbytes(&bheader.mode,      sizeof(short), 1);
    swapbytes(&bheader.ctcount,   sizeof(int),   1);
    swapbytes(&bheader.lpval,     sizeof(float), 1);
    swapbytes(&bheader.rpval,     sizeof(float), 1);
    swapbytes(&bheader.lvl,       sizeof(float), 1);
    swapbytes(&bheader.tlt,       sizeof(float), 1);
  }


  /*================+
  | WRITE PHASEFILE |
  +================*/

  (void) fwrite(&fheader, sizeof(fheader), 1, phfil);
  (void) fwrite(&bheader, sizeof(bheader), 1, phfil);
  if (swap)
  {
    if (debug)
    {
      (void) printf(
	     "swapping bytes in spectrum for \"phasefile\" (%d words)\n", fn);
    }
    swapbytes(spectrum, sizeof(float), fn);
  }
  (void) fwrite(spectrum, sizeof(float), fn, phfil);
  (void) fclose(phfil);
  if (swap)
  {
    if (debug)
    {
      (void) printf("swapping BACK bytes in spectrum (%d words)\n", fn);
    }
    swapbytes(spectrum, sizeof(float), fn);
  }


  /*-----------------------------------------+
  | swap BACK bytes in headers, if necessary |
  +-----------------------------------------*/
  if (swap)
  {
    if (debug)
    {
      (void) printf("Swapping BACK bytes in headers\n");
    }
    swapbytes(&fheader.nblocks,   sizeof(int),   1);
    swapbytes(&fheader.ntraces,   sizeof(int),   1);
    swapbytes(&fheader.np,        sizeof(int),   1);
    swapbytes(&fheader.ebytes,    sizeof(int),   1);
    swapbytes(&fheader.tbytes,    sizeof(int),   1);
    swapbytes(&fheader.bbytes,    sizeof(int),   1);
    swapbytes(&fheader.vers_id,   sizeof(short), 1);
    swapbytes(&fheader.status,    sizeof(short), 1);
    swapbytes(&fheader.nbheaders, sizeof(int),   1);

    swapbytes(&bheader.scale,     sizeof(short), 1);
    swapbytes(&bheader.status,    sizeof(short), 1);
    swapbytes(&bheader.index,     sizeof(short), 1);
    swapbytes(&bheader.mode,      sizeof(short), 1);
    swapbytes(&bheader.ctcount,   sizeof(int),   1);
    swapbytes(&bheader.lpval,     sizeof(float), 1);
    swapbytes(&bheader.rpval,     sizeof(float), 1);
    swapbytes(&bheader.lvl,       sizeof(float), 1);
    swapbytes(&bheader.tlt,       sizeof(float), 1);
  }

  if (debug)
  {
    (void) printf("Header information in \"phasefile\":\n");
    (void) printf("    fheader.nblocks:    %8d\n",     fheader.nblocks);
    (void) printf("    fheader.ntraces:    %8d\n",     fheader.ntraces);
    (void) printf("    fheader.np:         %8d\n",     fheader.np);
    (void) printf("    fheader.ebytes:     %8d\n",     fheader.ebytes);
    (void) printf("    fheader.tbytes:     %8d\n",     fheader.tbytes);
    (void) printf("    fheader.bbytes:     %8d\n",     fheader.bbytes);
    (void) printf("    fheader.vers_id:    %8d\n",     fheader.vers_id);
    (void) printf("    fheader.status:     %8d\n",     fheader.status);
    (void) printf("    fheader.nbheaders:  %8d\n\n",   fheader.nbheaders);
    (void) printf("    bheader.scale:      %8d\n",     bheader.scale);
    (void) printf("    bheader.status:     %8d\n",     bheader.status);
    (void) printf("    bheader.index:      %8d\n",     bheader.index);
    (void) printf("    bheader.mode:       %8d\n",     bheader.mode);
    (void) printf("    bheader.ctcount:    %8d\n",     bheader.ctcount);
    (void) printf("    bheader.lpval:      %8.6f\n",   bheader.lpval);
    (void) printf("    bheader.rpval:      %8.6f\n",   bheader.rpval);
    (void) printf("    bheader.lvl:        %8.6f\n",   bheader.lvl);
    (void) printf("    bheader.tlt:        %8.6f\n\n", bheader.tlt);
  }



  /*-----------------------------+
  | Adjust headers for data file |
  +-----------------------------*/
  fheader.np *= 2;
  fheader.tbytes        = (int)  (fheader.np * fheader.ebytes);
  fheader.bbytes        = (int)  (fheader.tbytes*fheader.ntraces +
			          sizeof(bheader));
  fheader.vers_id       = (short) 0x78c1; /* (taken from example) */
  fheader.status += S_COMPLEX;

  bheader.status += (S_COMPLEX + S_SECND);

  if (debug)
  {
    (void) printf("New header information for \"data\":\n");
    (void) printf("    fheader.nblocks:    %8d\n",     fheader.nblocks);
    (void) printf("    fheader.ntraces:    %8d\n",     fheader.ntraces);
    (void) printf("    fheader.np:         %8d ***\n", fheader.np);
    (void) printf("    fheader.ebytes:     %8d\n",     fheader.ebytes);
    (void) printf("    fheader.tbytes:     %8d ***\n", fheader.tbytes);
    (void) printf("    fheader.bbytes:     %8d ***\n", fheader.bbytes);
    (void) printf("    fheader.vers_id:    %8d ***\n", fheader.vers_id);
    (void) printf("    fheader.status:     %8d ***\n", fheader.status);
    (void) printf("    fheader.nbheaders:  %8d\n\n",   fheader.nbheaders);
    (void) printf("    bheader.scale:      %8d\n",     bheader.scale);
    (void) printf("    bheader.status:     %8d ***\n", bheader.status);
    (void) printf("    bheader.index:      %8d\n",     bheader.index);
    (void) printf("    bheader.mode:       %8d\n",     bheader.mode);
    (void) printf("    bheader.ctcount:    %8d\n",     bheader.ctcount);
    (void) printf("    bheader.lpval:      %8.6f\n",   bheader.lpval);
    (void) printf("    bheader.rpval:      %8.6f\n",   bheader.rpval);
    (void) printf("    bheader.lvl:        %8.6f\n",   bheader.lvl);
    (void) printf("    bheader.tlt:        %8.6f\n\n", bheader.tlt);
  }


  /*-------------------+
  | try to open "data" |
  +-------------------*/
  data = fopen(datname, "w");
  if (data == NULL)
  {
    (void) fprintf(stderr, "import1Dspec:  problem opening output file %s\n",
		   datname);
    (void) fclose(ascii);
    exit(1);
  }


  /*-----------------------+
  | build complex spectrum |
  +-----------------------*/
  j = 0;
  for (i = 0; i < fn; i++)
  {
    cspectrum[j] = spectrum[i];
    j += 2;
  }


  /*------------------------------------+
  | swap bytes in headers, if necessary |
  +------------------------------------*/
  if (swap)
  {
    if (debug)
    {
      (void) printf("Re-swapping bytes in headers for writing\n");
    }
    swapbytes(&fheader.nblocks,   sizeof(int),   1);
    swapbytes(&fheader.ntraces,   sizeof(int),   1);
    swapbytes(&fheader.np,        sizeof(int),   1);
    swapbytes(&fheader.ebytes,    sizeof(int),   1);
    swapbytes(&fheader.tbytes,    sizeof(int),   1);
    swapbytes(&fheader.bbytes,    sizeof(int),   1);
    swapbytes(&fheader.vers_id,   sizeof(short), 1);
    swapbytes(&fheader.status,    sizeof(short), 1);
    swapbytes(&fheader.nbheaders, sizeof(int),   1);

    swapbytes(&bheader.scale,     sizeof(short), 1);
    swapbytes(&bheader.status,    sizeof(short), 1);
    swapbytes(&bheader.index,     sizeof(short), 1);
    swapbytes(&bheader.mode,      sizeof(short), 1);
    swapbytes(&bheader.ctcount,   sizeof(int),   1);
    swapbytes(&bheader.lpval,     sizeof(float), 1);
    swapbytes(&bheader.rpval,     sizeof(float), 1);
    swapbytes(&bheader.lvl,       sizeof(float), 1);
    swapbytes(&bheader.tlt,       sizeof(float), 1);
  }


  /*=============+
  | WRITE "DATA" |
  +=============*/
  (void) fwrite(&fheader, sizeof(fheader), 1, data);
  (void) fwrite(&bheader, sizeof(bheader), 1, data);
  if (swap)
  {
    if (debug)
    {
      (void) printf(
	     "swapping bytes in spectrum for \"data\" (%d words)\n", 2*fn);
    }
    swapbytes(cspectrum, sizeof(float), 2*fn);
  }
  (void) fwrite(cspectrum, sizeof(float), 2*fn, data);
  (void) fclose(data);


  /* generate standard output */
  (void) printf("Generated phasefile \"%s\"\n", phfname);
  (void) printf("   from ASCII file \"%s\"\n", asciiname);
  if (xy)
    (void) printf("   input file found to contain X,Y .. X,Y data\n");
  else
    (void) printf("   input file found to contain Y .. Y data\n");
  (void) printf("Output file created with %ld real points\n", fn);
  if (vs != 1.0)
    (void) printf("   output scaled down by vs=%g\n", vs);
  (void) printf("Auxiliary data file \"%s\" generated\n", datname);

  return(0);
}
