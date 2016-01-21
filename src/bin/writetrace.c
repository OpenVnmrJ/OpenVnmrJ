/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* writetrace - write 1D trace in ASCII format */

/* syntax:  writetrace <-debug> <-f2> spec_file <trace <offset <points>>> */

/*      writetrace - prints the specified 1D trace (or a fraction thereof)
		in single column (Y) format; Y is the unscaled value from
	   	"phasefile". Called by the macro "writetrace".

        compilation with
                cc -O -o writetrace writetrace.c -m32

 Revision history:
   2006-02-18 - r.kyburz, started (based on jdxspec.c)
   2006-03-15 - r.kyburz, fixed byte swapping for Intel-based Macs (courtesy
		Pascal Mercier, Chenomx)
   2008-07-08 - r.kyburz, enhanced PC/Linux architecture detection


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>


static char rev[] =     "writetrace.c 3.5";
static char revdate[] = "2009-03-17_09:34";

static char cmd[] = "writetrace";
static int debug = 0;
static int swap = 0;

#define INT   1
#define FLOAT 0

FILE *spec;


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



/*--------------+
| ============= |
| MAIN FUNCTION |
| ============= |
+--------------*/

int main (argc, argv)
  int argc;
  char *argv[];
{

  /*------------+
  | Definitions |
  +------------*/

  /* defining structures */
  struct fileHeader
  {
    int nblocks, ntraces, np, ebytes, tbytes, bbytes;
    short vers_id, status;
    int nblockheaders;
  };
  struct blockHeader
  {
    short scale, status, index, mode;
    int ctcount;
    float lpval, rpval, lvl, tlt;
  };

  /* defining variables */
  struct fileHeader fheader;
  struct blockHeader bheader;
  int min_arg = 1;
  int f2mode = 0, byteoffset;
  int offset = 0, points = 1073741824, f2offset = 0, skipblocks = 0;
  int i, ok, tmp, trace = 1;
  char *specname, arch[64];
  float *spectrum;
  struct utsname *s_uname;


  /*-------------------+
  | checking arguments |
  +-------------------*/

  /* number of arguments: 1 at least, 2 maximum */
  min_arg = 1;

  i = 1;
  while ((i < argc) && (argv[i][0] == '-'))
  {
    /* -v / -version option prints version and exits, ignoring other args */
    if ((!strcasecmp(argv[i], "-v")) || (!strcasecmp(argv[i], "-version")))
    {
      (void) printf("%s (%s)\n", rev, revdate);
      exit(0);
    }

    if (!strcasecmp(argv[i],"-debug"))
    {
      debug = 1;
      min_arg++;
    }
    else if (!strcasecmp(argv[i],"-f2"))
    {
      f2mode = 1;
      min_arg++;
    }
    else if (argv[i][0] == '-')
    {
      (void) fprintf(stderr,
        "Usage:  writetrace <-debug> <-f2> file <trace# <offset <points>>>\n");
      return(1);
    }
    i++;
  }

  /* number of (non-option) arguments: 1 at least, 4 maximum */
  if ((argc < (min_arg + 1)) || (argc > (min_arg + 4)))
  {
    (void) fprintf(stderr,
        "Usage:  writetrace <-debug> <-f2> file <trace# <offset <points>>>\n");
    return(1);
  }

  specname = argv[min_arg];

  /* arg2: trace number (default: 1) */
  if (argc > (min_arg + 1))
  {
    ok = sscanf(argv[min_arg + 1], "%d", &trace);
    if (!ok)
    {
      (void) fprintf(stderr,
        "Usage:  writetrace <-debug> <-f2> file <trace# <offset <points>>>\n");
      (void) fprintf(stderr,"        argument #%d must be an integer\n",
	     min_arg + 1);
      return(1);
    }
  }
  trace--;

  /* arg3: offset */
  if (argc > (min_arg + 2))
  {
    if (sscanf(argv[min_arg + 2], "%d", &offset) == 0)
    {
      (void) fprintf(stderr,
        "Usage:  writetrace <-debug> <-f2> file <trace# <offset <points>>>\n");
      (void) fprintf(stderr,"        argument #%d must be an integer\n",
	     min_arg + 2);
      return(1);
    }
  }

  /* arg4: points */
  if (argc > (min_arg + 3))
  {
    if (sscanf(argv[min_arg + 3], "%d", &points) == 0)
    {
      (void) fprintf(stderr,
        "Usage:  writetrace <-debug> <-f2> file <trace# <offset <points>>>\n");
      (void) fprintf(stderr,"        argument #%d must be an integer\n",
	     min_arg + 3);
      return(1);
    }
  }

  if (debug)
  {
    (void) fprintf(stderr, "Parameters after reading args:\n");
    (void) fprintf(stderr, "    trace#      %8d\n", trace);
    (void) fprintf(stderr, "    offset      %8d\n", offset);
    (void) fprintf(stderr, "    points      %8d\n", points);
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


  /*------------------+
  | check file access |
  +------------------*/

  /* open spec file and read file header */
  spec = fopen(specname, "r");
  if (spec == NULL)
  {
    (void) fprintf(stderr,
		"writetrace:  problem opening file %s\n", specname);
    return(1);
  }
  (void) fread(&fheader, sizeof(fheader), 1, spec);
  if (swap)
  {
    swapbytes(&fheader.nblocks,       sizeof(int),   1);
    swapbytes(&fheader.ntraces,       sizeof(int),   1);
    swapbytes(&fheader.np,            sizeof(int),   1);
    swapbytes(&fheader.ebytes,        sizeof(int),   1);
    swapbytes(&fheader.tbytes,        sizeof(int),   1);
    swapbytes(&fheader.bbytes,        sizeof(int),   1);
    swapbytes(&fheader.vers_id,       sizeof(short), 1);
    swapbytes(&fheader.status,        sizeof(short), 1);
    swapbytes(&fheader.nblockheaders, sizeof(int),   1);
  }


  if (debug)
  {
    (void) fprintf(stderr,
		"fheader.nblocks       = %8d\n", fheader.nblocks);
    (void) fprintf(stderr,
		"fheader.ntraces       = %8d\n", fheader.ntraces);
    (void) fprintf(stderr,
		"fheader.np            = %8d\n", fheader.np);
    (void) fprintf(stderr,
		"fheader.ebytes        = %8d\n", fheader.ebytes);
    (void) fprintf(stderr,
		"fheader.tbytes        = %8d\n", fheader.tbytes);
    (void) fprintf(stderr,
		"fheader.bbytes        = %8d\n", fheader.bbytes);
    (void) fprintf(stderr,
		"fheader.vers_id       = %8d\n", fheader.vers_id);
    (void) fprintf(stderr,
		"fheader.status        =     %04x\n", fheader.status);
    (void) fprintf(stderr,
		"fheader.nblockheaders = %8d\n", fheader.nblockheaders);
  }

  if (f2mode)
  {
    tmp = fheader.np;
    fheader.np = fheader.ntraces * fheader.nblocks;
    fheader.ntraces = tmp / fheader.nblocks;
    f2offset = fheader.nblocks;
    if (debug)
    {
      (void) fprintf(stderr,
		"\nF2 mode: swapped columns and rows:\n");
      (void) fprintf(stderr,
		"fheader.ntraces       = %8d\n", fheader.ntraces);
      (void) fprintf(stderr,
		"fheader.np            = %8d\n", fheader.np);
      (void) fprintf(stderr,
		"f2 offset             = %8d block(s)\n", f2offset);
    }
  }

  if (offset < 0)
  {
    offset = 0;
    if (debug)
    {
      (void) fprintf(stderr, "setting offset to 0\n");
    }
  }
  else if (offset > fheader.np)
  {
    offset = fheader.np;
    if (debug)
    {
      (void) fprintf(stderr, "setting offset to %d\n", offset);
      (void) fprintf(stderr,
		"   can't be bigger than number of points / trace\n");
    }
  }

  if (points > fheader.np)
  {
    points = fheader.np;
  }
  if ((points + offset) > fheader.np)
  {
    points = fheader.np - offset;
    if (debug)
    {
      (void) fprintf(stderr,
                "setting number of points to %d\n", points);
    }
  }
  if ((int) trace >= fheader.nblocks * fheader.ntraces)
  {
    trace = (int) ((fheader.nblocks * fheader.ntraces) - 1);
  }

  if (debug)
  {
    (void) fprintf(stderr, "\nParameters after checking data header:\n");
    (void) fprintf(stderr, "    trace#      %8d\n", trace);
    (void) fprintf(stderr, "    offset      %8d\n", offset);
    (void) fprintf(stderr, "    points      %8d\n\n", points);
  }

  skipblocks = f2offset + (int) (trace / fheader.ntraces);
  trace = trace % fheader.ntraces;
  byteoffset = sizeof(fheader) +
	       skipblocks*fheader.bbytes +
	       sizeof(bheader)*fheader.nblockheaders +
	       trace*fheader.np*fheader.ebytes + offset*fheader.ebytes;
	       
  if (debug)
  {
    (void) fprintf(stderr, "skipblocks:     %8d\n", skipblocks);
    (void) fprintf(stderr, "trace in block: %8d\n", trace);
    (void) fprintf(stderr, "f2offset:       %8d\n", f2offset);
    (void) fprintf(stderr, "byteoffset:     %8d\n\n", byteoffset);
  }
             

  /* set file pointers */
  (void) fseek(spec, byteoffset, 0);


  /*====================+
  || do the conversion ||
  +====================*/

  if ((fheader.status & 0x8) != 0)	/* float spectrum */
  {
    spectrum = (float *) malloc(fheader.np * fheader.ebytes);
    ok = fread(spectrum, fheader.np * fheader.ebytes, 1, spec);
    if (!ok)
    {
      (void) fprintf(stderr,
		"writetrace: problems reading data from %s\n", specname);
      exit(1);
    }
    (void) fclose(spec);
    for (i = offset; i < (offset + points); i++)
    {
      if (swap)
      {
        swapbytes(&spectrum[i], sizeof(float), 1);
      }
      printf("%.6e\n", spectrum[i]);
    }
  }
  else
  {
    (void) fprintf(stderr, "writetrace: file has incorrect format, aborted.\n");
    exit(1);
  }

  return(0);
}
