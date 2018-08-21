/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* listparam - a tool for parsing parameter (sub-)sets in simpler format */

/*
   1995-03-25 - r.kyburz, started
   1997-01-10 - r.kyburz, fixed JCAMP-DX format, added unit in comment
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char rev[] =     "listparam.c 3.2";
static char revdate[] = "2009-03-17_09:34";


/* define parameter header constants */
#define OK	0
#define ERROR	1

#define UNDEF 	0
#define REAL	1
#define STRING	2

#define DELAY	3
#define FLAGPAR	4
#define FREQPAR	5
#define PULSE	6
#define INTEGER	7

#define NONE	0
#define SAMPLE	1
#define ACQPAR	2
#define PROCPAR	3
#define DISPLAY	4
#define SPSIM	5
#define JCAMP_SPEC	32765
#define JCAMP		32766
#define ALL	32767

#define OFF 0
#define ON  1

#define tprint (void) printf

/* define structures, global parameters */
struct parameter_header
{
  char name[64];
  int subtype, basictype;
  double maxvalue, minvalue, stepsize;
  int Ggroup, Dgroup, protection, active, intptr;
  int numval;
};

struct parameter_header parhd;
FILE *parfile;
char *fname;
int dumpflag,
    linepos = 0,
    jcampflag = 0,
    testval = ACQPAR;


/*----------------------------------+
| displayhelp() - display help info |
+----------------------------------*/
void displayhelp()
{
  (void) fprintf(stderr, "Usage:  listparam filename <parameter_group>\n");
  (void) fprintf(stderr, "\n   where parameter_group is one of\n");
  (void) fprintf(stderr, "	all:         ");
  (void) fprintf(stderr, "list all parameters\n");
  (void) fprintf(stderr, "	acquisition: ");
  (void) fprintf(stderr, "list acquisition parameters\n");
  (void) fprintf(stderr, "	processing:  ");
  (void) fprintf(stderr, "list processing parameters\n");
  (void) fprintf(stderr, "	display:     ");
  (void) fprintf(stderr, "list display parameters\n");
  (void) fprintf(stderr, "	sample:      ");
  (void) fprintf(stderr, "list sample parameters\n");
  (void) fprintf(stderr, "	spsim:       ");
  (void) fprintf(stderr, "list spin simulation parameters\n");
  (void) fprintf(stderr, "	JCAMP:       ");
  (void) fprintf(stderr, "list acquisition parameters (JCAMP-DX, FID)\n");
  (void) fprintf(stderr, "	JS:          ");
  (void) fprintf(stderr, "list acq. & proc. parameters (JCAMP-DX, spectra)\n");
  (void) fprintf(stderr, "\n\n");
}

/*------------------------------------------+
| readheader() - read parameter header line |
+------------------------------------------*/
int readheader()
{
  char c;
  int i = 0;

  c = (char) fgetc(parfile);
  while ((c == ' ') || (c == '\n'))
    c = (char) fgetc(parfile);
  while ((c != ' ') && (c != EOF) && (c != '\n'))
  {
    parhd.name[i++] = c;
    c = (char) fgetc(parfile);
  }
  if (c == EOF) return(0);
  parhd.name[i] = '\0';
  i = fscanf(parfile, "%d %d %lg %lg %lg %d %d %d %d %d\n%d ",
	&parhd.subtype, &parhd.basictype,
	&parhd.maxvalue, &parhd.minvalue, &parhd.stepsize,
	&parhd.Ggroup, &parhd.Dgroup, &parhd.protection,
	&parhd.active, &parhd.intptr, &parhd.numval);
  return(i);
}


/*--------------------------------------------------------------+
| dumptype() - print out information from parameter header line |
+--------------------------------------------------------------*/
void dumptype()
{
  if (dumpflag)
  {
    if (testval == ALL)
    {
      switch (parhd.Ggroup)
      {
	case NONE:	tprint("(none)      "); break;
	case SAMPLE:	tprint("(sample)    "); break;
	case ACQPAR:	tprint("(acq)       "); break;
	case PROCPAR:	tprint("(proc)      "); break;
	case DISPLAY:	tprint("(display)   "); break;
	case SPSIM:	tprint("(spsim)     "); break;
	default:	tprint("(Ggroup %d) ", parhd.Ggroup); break;
      }
    }
    if (parhd.active == OFF)
      tprint("NOT_USED   ");
  }
}


/*-------------------------------------------------+
| dumpnumber() - print out numeric parameter value |
+-------------------------------------------------*/
int dumpnumber()
{
  char c = (char) fgetc(parfile);

  if (c == EOF) return(ERROR);
  while (c == ' ')
  {
    c = (char) fgetc(parfile);
    if (c == EOF) return(ERROR);
  } 
  while ((c != ' ') && (c != '\n') && (c != EOF))
  { 
    (void) putchar(c);
    linepos++;
    c = (char) fgetc(parfile);
  } 
  if (!jcampflag)
  {
    switch (parhd.subtype)
    {
      case DELAY:	tprint(" sec\n"); break;
      case FREQPAR:	tprint(" Hz\n"); break;
      case PULSE:	tprint(" usec\n"); break;
      default:		tprint("\n"); break;
    }
  }
  return(OK);
}


/*--------------------------------------------+
| skipnumber() - skip numeric parameter value |
+--------------------------------------------*/
int skipnumber()
{
  char c = (char) fgetc(parfile);
 
  if (c == EOF) return(ERROR);
  while ((c == ' ') || (c == '\n'))
  {
    c = (char) fgetc(parfile);
    if (c == EOF) return(ERROR);
  }
  while ((c != ' ') && (c != '\n'))
  {
    c = (char) fgetc(parfile);
    if (c == EOF) return(ERROR);
  }
  return(OK);
}


/*------------------------------------------------+
| dumpstring() - print out parameter string value |
+------------------------------------------------*/
int dumpstring()
{
  char c, lastc;

  lastc = c = (char) fgetc(parfile);
  if (c == EOF) return(ERROR); 
  while ((c == ' ') || (c == '\n'))
  {
    c = (char) fgetc(parfile);
    if (c == EOF) return(ERROR);
  } 
  if (c == '"') 
  {
    if (jcampflag)
      (void) putchar('<'); 
    else
      (void) putchar(c); 
    lastc = c; 
    c = (char) fgetc(parfile); 
    if (c == EOF) return(ERROR);
  } 
  while ((c != '"') || ((c == '"') && (lastc == '\\')))
  { 
    (void) putchar(c);
    lastc = c;  
    c = (char) fgetc(parfile);
    if (c == EOF) return(ERROR);
  } 
  if (jcampflag)
    (void) putchar('>'); 
  else
  {
    (void) putchar('"');
    (void) putchar('\n');
  }
  return(OK);
}


/*-------------------------------------------+
| skipstring() - skip parameter string value |
+-------------------------------------------*/
int skipstring()
{
  char c, lastc;

  lastc = c = (char) fgetc(parfile);
  if (c == EOF) return(ERROR);
  while ((c == ' ') || (c == '\n'))
  {
    c = (char) fgetc(parfile);
    if (c == EOF) return(ERROR);
  }
  if (c == '"')
  {
    lastc = c;
    c = (char) fgetc(parfile);
    if (c == EOF) return(ERROR);
  }
  while ((c != '"') || ((c == '"') && (lastc == '\\')))
  {
    lastc = c;
    c = (char) fgetc(parfile);
    if (c == EOF) return(ERROR);
  }
  return(OK);
}


/*--------------------------------------------+
| dumpvalue() - print or skip parameter value |
+--------------------------------------------*/
int dumpvalue()
{
  if (parhd.basictype == STRING)
  {
    if (dumpflag)
      return(dumpstring());
    else
      return(skipstring());
  }
  else
  {
    if (dumpflag)
      return(dumpnumber());
    else
      return(skipnumber());
  }
}


/*----------------------------------------------------+
| skipenumerals() - skip enumerative parameter values |
+----------------------------------------------------*/
int skipenumerals()
{
  int i, res, numenum;

  res = fscanf(parfile, "%d", &numenum);
  if (res == 0)
  {
    (void) fprintf(stderr, "listparam: problem reading number of enumerals\n");
    (void) fclose(parfile);
    return(ERROR);
  }
  for (i = 1; i <= numenum; i++)
  {
    if (parhd.basictype == STRING)
      res = skipstring();
    else
      res = skipnumber();
    if (res != OK)
    {
      (void) fprintf(stderr, "listparam: problem reading enumerals\n");
      (void) fclose(parfile);
      return(ERROR);
    }
  }
  return(OK);
}


/*-------------------------------------+
| skipvalues() - skip parameter values |
+-------------------------------------*/
int skipvalues()
{
  int i, res;

  for (i = 1; i <= parhd.numval; i++)
  {
    if (parhd.basictype == STRING)
      res = skipstring();
    else
      res = skipnumber();
    if (res != OK)
    {
      (void) fprintf(stderr, "listparam: problem skipping values\n");
      (void) fclose(parfile);
      return(ERROR);
    }
  }
  return(OK);
}


/*-----------------------------------------------------+
| dump_parameter() - print out (or skip) one parameter |
+-----------------------------------------------------*/
int dump_parameter()
{
  dumptype();
  if (dumpvalue() != OK)
  {
    (void) fprintf(stderr, "listparam: problem reading parameter value\n");
    (void) fclose(parfile);
    return(ERROR);
  }
  return(OK);
}


/*---------+
|  main()  |
+---------*/
int main (argc, argv)
  int argc;
  char *argv[];
{
  int i, k, len;
  char parname[64];

  /* report version information with "-v" / "-version" argument */
  if (argc >= 2)
  {
    if ((!strcasecmp(argv[1], "-v")) || (!strcasecmp(argv[1], "-version")))
    {
      (void) printf("%s (%s)\n", rev, revdate);
      exit(0);
    }
  }

  /* check arguments */
  if ((argc < 2) || (argc > 3))
  {
    displayhelp();
    return(ERROR);
  }
  fname = argv[1];
  if (argc > 2)
  {
    if      (! strncasecmp(argv[2], "al", 2)) testval = ALL;
    else if (! strncasecmp(argv[2], "no", 2)) testval = NONE;
    else if (! strncasecmp(argv[2], "un", 2)) testval = NONE;
    else if (! strncasecmp(argv[2], "sa", 2)) testval = SAMPLE;
    else if (! strncasecmp(argv[2], "ac", 2)) testval = ACQPAR;
    else if (! strncasecmp(argv[2], "pr", 2)) testval = PROCPAR;
    else if (! strncasecmp(argv[2], "di", 2)) testval = DISPLAY;
    else if (! strncasecmp(argv[2], "sp", 2)) testval = SPSIM;
    else if (! strncasecmp(argv[2], "jc", 2)) testval = JCAMP;
    else if (! strncasecmp(argv[2], "js", 2)) testval = JCAMP_SPEC;
    jcampflag = ((testval == JCAMP) || (testval == JCAMP_SPEC));
  }
  else
  {
    if ((argc == 2) && (! strcmp(argv[1], "help")))
    {
      displayhelp();
      return(OK);
    }
  }

  /* open parameter file */
  parfile = fopen(fname, "r");
  if (parfile == NULL)
  {
    (void) fprintf(stderr, "listparam: problem opening file %s\n", fname);
    return(ERROR);
  }

  /* read & dump parameters */
  while (readheader())
  {
    dumpflag = ((testval == ALL) ||
		(parhd.Ggroup == testval) ||
		(parhd.Ggroup == NONE) ||
	  	((testval == JCAMP) && (parhd.active == ON) &&
                 ((parhd.Ggroup == ACQPAR) || (parhd.Ggroup == SAMPLE))) ||
		((testval == JCAMP_SPEC) && (parhd.active == ON) &&
                 ((parhd.Ggroup == ACQPAR) || (parhd.Ggroup == PROCPAR) ||
                  (parhd.Ggroup == SAMPLE))));
    if ((dumpflag) && (testval == JCAMP_SPEC))
    {
      if ((!strcmp(parhd.name,"lb1")) || (!strcmp(parhd.name,"sb1")) ||
	  (!strcmp(parhd.name,"sbs1")) || (!strcmp(parhd.name,"gf1")) ||
	  (!strcmp(parhd.name,"gfs1")) || (!strcmp(parhd.name,"awc1")) ||
	  (!strcmp(parhd.name,"lsfid1")) || (!strcmp(parhd.name,"phfid1")) ||
	  (!strcmp(parhd.name,"wtfile1")) || (!strcmp(parhd.name,"proc1")) ||
	  (!strcmp(parhd.name,"fn1")) || (!strcmp(parhd.name,"math")) ||
	  (!strcmp(parhd.name,"lsfrq1")))
      {
        dumpflag = 0;
      }
    }
    if (parhd.numval == 0)
    {
      (void) fprintf(stderr, "listparam: problem reading number of values\n");
      if ((dumpflag) && (!jcampflag))
      {
        tprint("%-15s ", parhd.name);
        dumptype();
	tprint("Parameter has got no value!\n");
      }
      (void) fclose(parfile);
      return(ERROR);
    }
    else if (dumpflag)
    {
      if (parhd.numval == 1)
      {
	if (jcampflag)
        {
          linepos = 0;
	  (void) strcpy(parname, parhd.name);
	  (void) strcat(parname, "=");
	  tprint("##$%-13s ", parname);
	}
	else
          tprint("%-15s ", parhd.name);
        if (dump_parameter()) return(ERROR);
	if (jcampflag)
        {
	  if ((parhd.subtype == DELAY) ||
	      (parhd.subtype == FREQPAR) ||
	      (parhd.subtype == PULSE))
	  {
	    while (linepos < 18)
	    {
	      (void) putchar(' ');
	      linepos++;
	    }
	  }
          switch (parhd.subtype)
          {
            case DELAY:	  tprint("$$ sec\n"); break;
            case FREQPAR: tprint("$$ Hz\n"); break;
            case PULSE:	  tprint("$$ usec\n"); break;
            default:	  tprint("\n"); break;
          }
        }
      }
      else
      {
	if (jcampflag)
	{
          linepos = 0;
	  (void) strcpy(parname, parhd.name);
	  (void) strcat(parname, "=");
	  tprint("##$%-13s ", parname);
	  tprint("(0..%d)  ", parhd.numval-1);
          for (i = 1; i <= parhd.numval; i++)
          {
            if (dump_parameter()) return(ERROR);
            if (i < parhd.numval)
	      tprint(" ");
	    else
            {
              switch (parhd.subtype)
              {
                case DELAY:   tprint("   $$ sec\n"); break;
                case FREQPAR: tprint("   $$ Hz\n"); break;
                case PULSE:   tprint("   $$ usec\n"); break;
                default:      tprint("\n"); break;
              }
            }
    	  }
	}
	else
   	{
          for (i = 1; i <= parhd.numval; i++)
          {
            len = strlen(parhd.name);
            if (parhd.numval > 999)
	    {
              tprint("%s[%04d] ", parhd.name, i);
	      len += 6;
            }
	    else if (parhd.numval > 99)
	    { 
              tprint("%s[%03d] ", parhd.name, i);
              len += 5; 
            } 
	    else if (parhd.numval > 9)
	    { 
              tprint("%s[%02d] ", parhd.name, i);
              len += 4; 
            } 
            else 
	    { 
              tprint("%s[%d] ", parhd.name, i);
              len += 3; 
            } 
	    for (k = 0; k < 15 - len; k++)
	      (void) putchar(' ');
            if (dump_parameter()) return(ERROR);
          }
        }
      }
    }
    else
      skipvalues();
    if (skipenumerals() != OK)
    {
      (void) fprintf(stderr, "listparam: problem reading enumerals\n");
      (void) fclose(parfile);
      return(ERROR);
    }
  }

  (void) fclose(parfile);
  return(OK);
}
