/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* jdxspec - write JCAMP-DX format ASCII files from VNMR spectrum */

/* syntax:  jdxspec <-opt> spec_file <trace> */

/*      jdxspec writes a file jdxspec into /vnmr/tmp; it also returns MAX
		and MIN values and other parameters required for JCAMP-DX.
                jdxspec is called by the svsdx macro.

        compilation with
                cc -O -o jdxspec jdxspec.c -m32

 Revision history:
   1991-12-10 - r.kyburz, started
   1997-01-09 - r.kyburz, fixed X axis offset by 1
   1997-01-10 - r.kyburz, fixed DIFDUP format
   1997-01-14 - r.kyburz, fixed INT format for pseudo-standard
   2002-05-14 - r.kyburz, fixed missing DUP digit at end
   2002-05-15 - r.kyburz, expanded for DIF, DUP, SQZ, PAC, FIX and TBL
                output options
   2006-02-17 - r.kyburz, expansions for PC/Linux (& Mac) architecture;
                improved argument treatment, added -debug, -v options
   2006-03-15 - r.kyburz, fixed byte swapping for Intel-based Macs (courtesy
                Pascal Mercier, Chenomx)
   2008-07-08 - r.kyburz, enhanced PC/Linux architecture detection

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>


static char rev[] =     "jdxspec.c 3.5";
static char revdate[] = "2009-03-17_09:34";

static char cmd[] = "jdxspec";
static int debug = 0;
static int swap = 0;

#define MAX 32767.0
#define MIN -32768.0
#define LMAX 8388607.0
#define LMIN -8388608.0

#define MAXLINE 76
#define DIFF 1
#define ABS  0
#define INT   1
#define FLOAT 0

FILE *spec, *outf;
int linepos, slen, val, lastval, repcount;
int dodif = 1, dodup = 1, dosqz = 1, dopac = 1, dofix = 1;
double lastxval;
char laststr[12], curstr[12];



/*----------------------------------------------+
| int2str - converts integer to JCAMP-DX string |
+----------------------------------------------*/

int int2str(v, s, diff)
int v, diff; char *s;
{
  int i, l;

  s[0] = '\0';
  if (dopac == 0)
  {
    (void) sprintf(s, " %d", v);
    l = strlen(s);
  }
  else if (dosqz == 0)
  {
    if (v < 0)
    {
      (void) sprintf(s, "%d", v);
    }
    else
    {
      (void) sprintf(s, "+%d", v);
    }
    l = strlen(s);
  }
  else
  {
    (void) sprintf(s, "%d", v);
    l = strlen(s);
    if (diff == 0)
    {
      if (s[0] == '-')
      {
        for (i = 1; i <= l; i++)
          s[i-1] = s[i];
        l--;
        switch (s[0])
        {
          case '1' : s[0] = 'a'; break;
          case '2' : s[0] = 'b'; break;
          case '3' : s[0] = 'c'; break;
          case '4' : s[0] = 'd'; break;
          case '5' : s[0] = 'e'; break;
          case '6' : s[0] = 'f'; break;
          case '7' : s[0] = 'g'; break;
          case '8' : s[0] = 'h'; break;
          case '9' : s[0] = 'i'; break;
        }
      }
      else
      {
        switch (s[0])
        {
          case '0' : s[0] = '@'; break;
          case '1' : s[0] = 'A'; break;
          case '2' : s[0] = 'B'; break;
          case '3' : s[0] = 'C'; break;
          case '4' : s[0] = 'D'; break;
          case '5' : s[0] = 'E'; break;
          case '6' : s[0] = 'F'; break;
          case '7' : s[0] = 'G'; break;
          case '8' : s[0] = 'H'; break;
          case '9' : s[0] = 'I'; break;
        }
      }
    }
    else /* not DIF */
    {
      if (s[0] == '-')
      {
        for (i = 1; i <= l; i++)
          s[i-1] = s[i];
        l--;
        switch (s[0])
        {
          case '1' : s[0] = 'j'; break;
          case '2' : s[0] = 'k'; break;
          case '3' : s[0] = 'l'; break;
          case '4' : s[0] = 'm'; break;
          case '5' : s[0] = 'n'; break;
          case '6' : s[0] = 'o'; break;
          case '7' : s[0] = 'p'; break;
          case '8' : s[0] = 'q'; break;
          case '9' : s[0] = 'r'; break;
        }
      }
      else
      {
        switch (s[0])
        {
          case '0' : s[0] = '%'; break;
          case '1' : s[0] = 'J'; break;
          case '2' : s[0] = 'K'; break;
          case '3' : s[0] = 'L'; break;
          case '4' : s[0] = 'M'; break;
          case '5' : s[0] = 'N'; break;
          case '6' : s[0] = 'O'; break;
          case '7' : s[0] = 'P'; break;
          case '8' : s[0] = 'Q'; break;
          case '9' : s[0] = 'R'; break;
        }
      }
    }
  }
  return(l);
}

/*--------------------------------+
| dump duplicate count (DUP mode) |
+--------------------------------*/
void dumpdup()
{
  int l;
  char dupstr[64];

  dupstr[0] = '\0';
  (void) sprintf(dupstr, "%d", repcount);
  l = strlen(dupstr);

  /* dump DUP character */
  switch (dupstr[0])
  {
    case '1': dupstr[0] = 'S'; break;
    case '2': dupstr[0] = 'T'; break;
    case '3': dupstr[0] = 'U'; break;
    case '4': dupstr[0] = 'V'; break;
    case '5': dupstr[0] = 'W'; break;
    case '6': dupstr[0] = 'X'; break;
    case '7': dupstr[0] = 'Y'; break;
    case '8': dupstr[0] = 'Z'; break;
    case '9': dupstr[0] = 's'; break;
  }
  (void) fprintf(outf, "%s", dupstr);
  linepos += l;
  repcount = 1;
  if (debug)
    (void) printf("dumped DUP, linepos = %d, ", linepos);
}


/*-------------+
| dump 1 value |
+-------------*/
void dumpvalue(xval, intflag)
double xval;
int intflag;
{
  char tmpstr[64];

  if (dofix == 0)
  {
    if (intflag == INT)
      (void) fprintf(outf, "%.0lf %d\n", xval, val);
    else
      (void) fprintf(outf, "%.3lf %d\n", xval, val);
  }
  else
  {
    if (linepos == 0)
    {
      if (intflag == INT)
        (void) sprintf(tmpstr, "%.0lf", xval);
      else
        (void) sprintf(tmpstr, "%.3lf", xval);
      (void) fprintf(outf, "%s", tmpstr);
      linepos = strlen(tmpstr);
      linepos += int2str(val, curstr, ABS);
      (void) fprintf(outf, "%s", curstr);
      (void) strcpy(laststr, curstr);
      if (debug)
        (void) printf("val  = %6d:   curstr = %s; linepos = %d\n",
	              val, curstr, linepos);
    }
    else
    {
      if (dodif == 1)
      {
        slen = int2str(val - lastval, curstr, DIFF);
        if (debug)
          (void) printf("diff = %6d:   curstr = %s; slen = %d\n",
	                val - lastval, curstr, slen);
      }
      else
      {
        slen = int2str(val, curstr, ABS);
        if (debug)
          (void) printf("val = %6d:   curstr = %s; slen = %d\n",
	                val, curstr, slen);
      }
      if ((strcmp(curstr, laststr)) || (dodup == 0))
      {
        if (debug)
          (void) printf("  new value, ");
        /* current string is not the same as the last one */
        if (repcount > 1) dumpdup();
        linepos += slen;
  
        if (linepos > MAXLINE)
        {
          if (dodif == 1)
          {
            if (intflag == INT)
              (void) sprintf(tmpstr, "%.0lf", lastxval);
            else
              (void) sprintf(tmpstr, "%.3lf", lastxval);
            (void) fprintf(outf, "\n%s", tmpstr);
            linepos = strlen(tmpstr);
            linepos += int2str(lastval, curstr, ABS);
            (void) fprintf(outf, "%s", curstr);
            linepos += int2str(val - lastval, curstr, DIFF);
            (void) fprintf(outf, "%s", curstr);
          }
          else
          {
            if (intflag == INT)
              (void) sprintf(tmpstr, "%.0lf", xval);
            else
              (void) sprintf(tmpstr, "%.3lf", xval);
            (void) fprintf(outf, "\n%s", tmpstr);
            linepos = strlen(tmpstr);
            linepos += int2str(val, curstr, ABS);
            (void) fprintf(outf, "%s", curstr);
          }
          if (debug)
            (void) printf("\nval  = %6d:   curstr = %s; linepos = %d\n",
		          val, curstr, linepos);
        }
        else
        {
          if (debug)
            (void) printf("dumping, last linepos = %d\n", linepos);
	  (void) fprintf(outf, "%s", curstr);
        }
        (void) strcpy(laststr, curstr);
      }
      else
      {
        repcount++;
        if (repcount >= 9)
        {
          (void) putc('s', outf);
          linepos++;
          repcount = 1;
	  laststr[0] = '\0';
        }
      }
    }
  }
  lastval = val;
  lastxval = xval;
}


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
  int first = 0, last = 0;
  int i_max = 0, i_min = 0;
  int maxval, minval;
  int min_arg = 1;
  int type_opt = 0;
  double factor = 1.0, xval,
	 firstx = 0.0,
	 lastx = 0.0,
	 max = 0.0,
	 min = 0.0;
  int i, ok, trace = 1;
  char *specname, outname[256], arch[64];
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
    else if (!strcasecmp(argv[i],"-difdup"))
    {
      dodif = dodup = dosqz = dopac = dofix = 1;
      min_arg++;
      type_opt++;
    }
    else if (!strcasecmp(argv[i],"-dif"))
    {
      dodup = 0;
      dodif = dosqz = dopac = dofix = 1;
      min_arg++;
      type_opt++;
    }
    else if (!strcasecmp(argv[i],"-dup"))
    {
      dodif = 0;
      dodup = dosqz = dopac = dofix = 1;
      min_arg++;
      type_opt++;
    }
    else if (!strcasecmp(argv[i],"-sqz"))
    {
      dodif = dodup = 0;
      dosqz = dopac = dofix = 1;
      min_arg++;
      type_opt++;
    }
    else if (!strcasecmp(argv[i],"-pac"))
    {
      dodif = dodup = dosqz = 0;
      dopac = dofix = 1;
      min_arg++;
      type_opt++;
    }
    else if (!strcasecmp(argv[i],"-fix"))
    {
      dodif = dodup = dosqz = dopac = 0;
      dofix = 1;
      min_arg++;
      type_opt++;
    }
    else if (!strcasecmp(argv[i],"-tbl"))
    {
      dodif = dodup = dosqz = dopac = dofix = 0;
      min_arg++;
      type_opt++;
    }
    else if (argv[i][0] == '-')
    {
      (void) fprintf(stderr,
        "Usage:  jdxspec <-debug> <-option> file <trace# <firstx <lastx>>>\n");
      return(1);
    }
    i++;
  }

  /* only 1 compression / format option allowed */
  if (type_opt > 1)
  {
    (void) fprintf(stderr,
        "Usage:  jdxspec <-debug> <-option> file <trace# <firstx <lastx>>>\n");
      (void) fprintf(stderr, "   where \"-option\" is ONE of\n");
      (void) fprintf(stderr, "        -difdup (default)\n");
      (void) fprintf(stderr, "        -dup\n");
      (void) fprintf(stderr, "        -dif\n");
      (void) fprintf(stderr, "        -sqz\n");
      (void) fprintf(stderr, "        -pac\n");
      (void) fprintf(stderr, "        -fix\n");
      (void) fprintf(stderr, "        -tbl\n");
      (void) fprintf(stderr, "\n");
    return(1);
  }

  /* number of arguments: 1 at least, 2 maximum */
  if ((argc < (min_arg + 1)) || (argc > (min_arg + 4)))
  {
    (void) fprintf(stderr,
        "Usage:  jdxspec <-debug> <-option> file <trace# <firstx <lastx>>>\n");
    return(1);
  }

  specname = argv[min_arg];
  (void) strcpy(outname,"/vnmr/tmp/jdxspec");

  /* arg2: trace number (default: 1) */
  if (argc > (min_arg + 1))
  {
    ok = sscanf(argv[min_arg + 1], "%d", &trace);
    if (!ok)
    {
      (void) fprintf(stderr,
                     "Usage:  jdxspec <-opt> file <trace# <firstx <lastx>>>\n");
      (void) fprintf(stderr,"        argument #%d must be numeric\n",
		     min_arg + 1);
      return(1);
    }
  }
  trace--;

  /* arg3: firstx */
  if (argc > (min_arg + 2))
  {
    if (sscanf(argv[min_arg + 2], "%lf", &firstx) == 0)
    {
      (void) fprintf(stderr,
                     "Usage:  jdxspec <-opt> file <trace# <firstx <lastx>>>\n");
      (void) fprintf(stderr,"        argument #%d must be numeric\n",
		     min_arg + 2);
      return(1);
    }
  }

  /* arg3: lastx */
  if (argc > (min_arg + 3))
  {
    if (sscanf(argv[min_arg + 3], "%lf", &lastx) == 0)
    {
      (void) fprintf(stderr,
                     "Usage:  jdxspec <-opt> file <trace# <firstx <lastx>>>\n");
      (void) fprintf(stderr,"        argument #%d must be numeric\n",
		     min_arg + 3);
      return(1);
    }
  }

  /* check for calling name */
  if (strcmp("jdxspec",argv[0]))
  {
    minval = LMIN;
    maxval = LMAX;
  }
  else
  {
    minval = MIN;
    maxval = MAX;
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
    (void) fprintf(stderr, "jdxspec:  problem opening file %s\n", specname);
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
    (void) printf("fheader.nblocks       = %ld\n", fheader.nblocks);
    (void) printf("fheader.ntraces       = %ld\n", fheader.ntraces);
    (void) printf("fheader.np            = %ld\n", fheader.np);
    (void) printf("fheader.ebytes        = %ld\n", fheader.ebytes);
    (void) printf("fheader.tbytes        = %ld\n", fheader.tbytes);
    (void) printf("fheader.bbytes        = %ld\n", fheader.bbytes);
    (void) printf("fheader.vers_id       = %d\n", fheader.vers_id);
    (void) printf("fheader.status        = %04x\n", fheader.status);
    (void) printf("fheader.nblockheaders = %ld\n", fheader.nblockheaders);
  }


  /* try to open temporary file */
  outf = fopen(outname, "w+");
  if (outf == NULL)
  {
    (void) fprintf(stderr,"jdxspec:  problem opening output file %s\n",
		   outname);
    (void) fclose(spec);
    return(1);
  }
  if (fheader.ntraces != 1)
  {
    (void) fprintf(stderr,"jdxspec:  not implemented for ntraces>1 (2D/3D)\n");
    (void) fclose(spec);
    (void) fclose(outf);
    return(1);
  }
  if ((int) trace >= fheader.nblocks) trace = (int) (fheader.nblocks - 1);

  /* set file pointers */
  (void) fseek(spec, sizeof(fheader) + trace*fheader.bbytes +
	             fheader.nblockheaders*sizeof(bheader), 0);


  /*====================+
  || do the conversion ||
  +====================*/

  if ((fheader.status & 0x8) != 0)	/* float spectrum */
  {
    spectrum = (float *) malloc(fheader.np * fheader.ebytes);
    (void) fread(spectrum, fheader.np * fheader.ebytes, 1, spec);
    (void) fclose(spec);
    for (i = 0; i < fheader.np; i++)
    {
      if (swap)
      {
        swapbytes(&spectrum[i], sizeof(float), 1);
      }
      if ((double) spectrum[i] > max) max = (double) spectrum[i];
      if ((double) spectrum[i] < min) min = (double) spectrum[i];
    }
    factor = max / maxval;
    if (min/(minval) > factor) factor = min/(minval);
    first = (int) ((double) spectrum[0] / factor);
    last  = (int) ((double) spectrum[fheader.np - 1] / factor);
    i_max = (int) (max / factor);
    i_min = (int) (min / factor);

    /* dump real part */
    linepos = lastval = 0;
    repcount = 1;
    for (i = 0; i < fheader.np; i++)
    {
      val = (int) ((double) spectrum[i] / factor);
      if ((firstx == 0.0) && (lastx == 0.0))
      {
        xval = (double) (fheader.np - i - 1);
        if (i == 0) lastxval = xval;
        dumpvalue(xval, INT);
      }
      else
      {
        xval = firstx - (firstx - lastx)*((double)i)/((double)(fheader.np - 1));
        if (i == 0) lastxval = xval;
        dumpvalue(xval, FLOAT);
      }
    }
    if (repcount > 1) dumpdup();
    if (dodif == 1)
    {
      val = (int) ((double) spectrum[fheader.np - 1] / factor);
      if ((firstx == 0.0) && (lastx == 0.0))
        (void) fprintf(outf, "\n%.0lf", lastxval);
      else
        (void) fprintf(outf, "\n%.3lf", lastxval);
      (void) int2str(val, curstr, ABS);
      if (debug)
        (void) printf("val  = %6d:   curstr = %s\n", val, curstr);
      (void) fprintf(outf, "%s    $$ checkpoint\n", curstr);
    }
    else if (dofix == 1)
    {
      (void) fprintf(outf, "\n");
    }
    (void) fclose(outf);
  }

  /* generate standard output */
  (void) printf("%d\n%d\n", first, last);
  (void) printf("%d\n%d\n", i_min, i_max);
  (void) printf("%11.9f\n", factor);
  
  return(0);
}
