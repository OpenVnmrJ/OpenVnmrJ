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
/*-----------------------------------------------------------------+
| readsctables  read tables from dps('-j') output (dpstable) for   |
| ============  C-to-SpinCAD pulse sequence conversion software    |
+------------------------------------------------------------------+
| USAGE:  	readsctables ct #tables < dpstable > outfile	   |
|								   |
| ARGUMENTS:    ct:      number of scans for which phase tables    |
|		         were extracted				   |
|	        #tables: number of tables present in input stream  |
|								   |
| 		IT IS ESSENTIAL THAT BOTH THE NUMBER OF SCANS (ct  |
|		= maximum table size), AS WELL AS THE NUMBER OF    |
|		TABLES IF FIRST PROPERLY EVALUATED, otherwise	   |
|		readsctables will not work properly!               |
|								   |
| COMPILATION:      cc -o readsctables readsctables.c [-Ddebug]	   |
+------------------------------------------------------------------+
| The INPUT is generated from within VNMR, using "dps('-j')",      |
| which generates a file "dpstable" in the current experiment,     |
| with the following format:					   |
|	ct= 1							   |
|	20  0							   |
|	26  0							   |
|	30  0 0							   |
|	..							   |
|	ct= 2							   |
|	20  2							   |
|	26  0							   |
|	30  1 3							   |
|	..							   |
| dps('-j') generates output for the currently specified number of |
| scans (nt); output is generated for any reference to real-time   |
| variable or phase table in the "dps" output (dpsdata in the	   |
| current esperiment); the first number in each line indicates the |
| line number in the file "dpsdata"				   |
+------------------------------------------------------------------+
| OUTPUT FORMAT: readsctables produces one line per table. Each    |
| starts with the "dpsdata" line number, followed by an ordinal    |
| number, indicating the table number on that "dpsdata" line.	   |
| These numbers are then followed by the actual table. The tables  |
| are reduced to minimum size by first repetitively dividing each  |
| table into two halves until a) the two halves are not identical, |
| or until the table size becomes an odd number (such as 1).       |
| Tables with less than 8 values are left as is; for longer tables |
| readsctables tries abbreviating the table using SpinCAD table    |
| short-hand syntax, where "0 ^4" stands for "0 0 0 0". The extra  |
| blank preceding the '^' will be removed by "vnmr2sc"		   |
|	20 1 0							   |
|	26 1 0							   |
|	30 1 0							   |
|	30 2 0							   |
|	31 1 1 1 3 3						   |
|	32 1 0 2						   |
|	34 1 1 1 3 3						   |
|	34 2 0 2						   |
|	38 1 0							   |
|	39 1 0^4 2^4						   |
|	...							   |
|	42 1 0^4 2^4						   |
+------------------------------------------------------------------+
| Revision History:						   |
|	1999-11-26: rk, first complete version			   |
|       2001-12-14: rk, added more debugging code, fixed getchar   |
|		  	and calloc calls			   |
+-----------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#ifdef debug
#define debug 1
#else
#define debug 0
#endif

#define TOKENSIZ 16
#define LINELEN 64

struct table
{
  int line, size;
  int *vals;
};

static int ct;
char token[TOKENSIZ], line[LINELEN];
struct table *tablearray;

/* read one input line */
void priv_getline()
{
  int i = 0;
  int c = getchar();
  while ((c != '\n') && (c != '\0') && (i < LINELEN) && (c != EOF))
  {
    line[i++] = (char) c;
    c = getchar();
  }
  line[i] = '\0';
  if (debug) fprintf(stderr, "priv_getline(): line=\"%s\"\n", line);
}


/*-----+
| MAIN |
+-----*/
main(argc,argv)
   int argc;
   char *argv[];
{
  int i, j, k, l, lineno, ntab, tables, compress;
  int lline = 0,
      lval = -32768,
      cline = 0,
      cval = 0,
      nlines = 0;
  char c;

  /* read arguments */
  if (argc < 3)
  {
    fprintf(stderr, "Usage:  readsctables ct #tables < dpstable > outfile\n");
    exit(1);
  }
  ct = atoi(argv[1]);
  tables = atoi(argv[2]);
  
  /* allocate memory for table header/pointer structure */
  if (debug)
    fprintf(stderr, "   Allocating %d bytes for table array\n",
	tables*sizeof(struct table));
  tablearray = (struct table *) malloc(tables*sizeof(struct table));
  if (tablearray == NULL)
  {
    fprintf(stderr, "   failed to allocate memory for table headers\n");
    exit(1);
  }

  if (debug) 
    fprintf(stderr, "   Reading input data:\n");

  /* skip first line */
  priv_getline();


  /*------------------------------+
  | now read first block of lines |
  +------------------------------*/
  ntab = 0;
  priv_getline();
  while ((line[0] != 'c') && (line[0] != '\0'))
  {
    i = 0;			/* line pointer */

    /* read dpsdata (target) line number */
    while ((line[i] != ' ') && (line[i] != '\0'))
    {
      token[i] = line[i]; 
      i++;
    }
    token[i] = '\0';
    lineno = atoi(token);

    /* read values on current line */
    while (line[i] != '\0')
    {
      /* skip white space */
      while (line[i] == ' ')
        i++;

      /* read token */
      j = 0; 			/* token pointer */
      while ((line[i] != ' ') && (line[i] != '\0') && (j < TOKENSIZ))
      {
        token[j] = line[i]; 
        i++; j++;
      }
      token[j] = '\0';

      if (token[0] != '\0')
      {
        /* allocate memory for each new table */
        tablearray[ntab].line = lineno;
        tablearray[ntab].size = ct;
        if (debug)
          fprintf(stderr, "   Allocating %d bytes for table #%d\n",
		ct*sizeof(int), ntab);
        tablearray[ntab].vals = (int *) calloc(ct, sizeof(int));
        if (tablearray[ntab].vals == NULL)
        {
	  fprintf(stderr, "failed to allocate memory at table #%d\n", ntab);
	  exit(1);
        }

	/* store first table value */
        tablearray[ntab].vals[0] = atoi(token);
        if (debug) 
          fprintf(stderr, "table #%2d: line=%d val[0]=%d\n", ntab,
	       	tablearray[ntab].line,
		tablearray[ntab].vals[0]);

	/* increment table counter */
        ntab++;
      }
    }

    /* increment line counter */
    nlines++;

    /* read next line */
    priv_getline();
  }


  /*---------------------------------------------------------+
  | read "ct" table values from the rest of the input stream |
  +---------------------------------------------------------*/
  /* read "ct" blocks of data */
  for (i = 1; (i < ct) && (line[0] != '\0'); i++)
  {
    /* reset table index */
    ntab = 0;

    /* read block line by line */
    for (j = 0; j < nlines; j++)
    {
      priv_getline();

      /* skip dpsdata line number (we assume properly organized input!) */
      k = 0;
      while ((line[k] != ' ') && (line[k] != '\0'))
        k++;

      /* skip whitespace after dpsdata line number */
      while (line[k] == ' ')
        k++;

      /* read values on current line */
      while (line[k] != '\0')
      {
        /* read token */
        l = 0;                    /* token pointer */
        while ((line[k] != ' ') && (line[k] != '\0') && (l < TOKENSIZ))
        {  
          token[l] = line[k];
          k++; l++;
        }  
        token[l] = '\0';

	/* skip whitespace after last token */
        while (line[k] == ' ')
          k++;

        /* store table value */
        if (token[0] != '\0')
        {
          tablearray[ntab].vals[i] = atoi(token);
          if (debug) 
            fprintf(stderr, "table #%2d: line=%d val[%d]=%d\n", ntab,
	       	  tablearray[ntab].line, i,
		  tablearray[ntab].vals[i]);

	  /* increment table index */
          ntab++;
        }
      }
    }

    /* read next line */
    priv_getline();
  }


  /*--------------------------------------------------+
  | debugging only: print tables prior to compression |
  +--------------------------------------------------*/
  if (debug) 
  {
    fprintf(stderr, "   Tables prior to compression:\n");
    for (i = 0; i < tables; i++)
    {
      fprintf(stderr, "table #%2d line=%d size=%d:", i,
	       	  tablearray[i].line, tablearray[i].size);
      for (j = 0; j < tablearray[i].size; j++)
	fprintf(stderr, " %d",tablearray[i].vals[j]);
      fprintf(stderr, "\n");
    }
  }


  /*-------------------------------------------------------+
  | compress tables, step 1: repetitively split table into |
  | two halves and check whether the two half-tables are   |
  | identical. terminate when odd table size reached or    |
  | when the two halves are not identical                  |
  +-------------------------------------------------------*/
  if (ct > 1)
  {
    for (i = 0; i < ntab; i++)
    {
      /* don't even start if table size is odd number */
      if ((ct % 2) == 0) compress = 1; else compress = 0;

      for (j = ct/2; (j > 0) && (compress == 1); j /= 2)
      {
	/* compare (size/2) values */
        for (k = 0; (k < j) && (compress == 1); k++)
        {
	  if (tablearray[i].vals[k] != tablearray[i].vals[k + j])
	    compress = 0;
        }

	/* half-tables were identical, reduce size by factor of 2 */
	if (compress == 1) tablearray[i].size /= 2;

	/* terminate when odd size (e.g.: 1) reached */
        if ((tablearray[i].size % 2) == 1) compress = 0;
      }
    }
  }


  /*-----------------------------------------------+
  | debugging only: print tables after compression |
  +-----------------------------------------------*/
  if (debug) 
  {
    fprintf(stderr, "   Tables after compression:\n");
    for (i = 0; i < tables; i++)
    {
      fprintf(stderr, "table #%2d line=%d size=%d:", i,
	       	  tablearray[i].line, tablearray[i].size);
      for (j = 0; j < tablearray[i].size; j++)
	fprintf(stderr, " %d",tablearray[i].vals[j]);
      fprintf(stderr, "\n");
    }
    fprintf(stderr, "   Regular command output:\n");
  }


  /*-----------------------------------------------+
  | print out resulting tables, one table per line |
  +-----------------------------------------------*/
  for (i = 0; i < ntab; i++)
  {
    /* set table number (per line) to one if new line */
    if (tablearray[i].line != lline)
      cline = 1;
    /* otherwise increment table number */
    else
      cline++;

    /* remember dpsdata line  number for next table */
    lline = tablearray[i].line;

    /* start each line with dpsdata line number and table count */
    printf("%d %d", tablearray[i].line, cline);

    /* for tables with less than 8 values print values as found */
    if (tablearray[i].size < 8)
    {
      for (j = 0; j < tablearray[i].size; j++)
        printf(" %d", tablearray[i].vals[j]);
      printf("\n");
    }
    else
    {
      /*----------------------------------------------------+
      | tables with 8 or more values: check whether SpinCAD |
      | shorthand syntax (0^4 2^4) applies                  |
      +----------------------------------------------------*/
      for (j = 0; j < tablearray[i].size; j++)
      {
	/* first value in table */
        if (j == 0)
        {
          /* print first value */
	  printf(" %d", tablearray[i].vals[j]);

	  /* remember last value */
	  lval = tablearray[i].vals[j];

	  /* initialize value count */
	  cval = 1;
	}
	else	/* other values in table */
	{
	  /* value is different from last one */
	  if (tablearray[i].vals[j] != lval)
	  {
	    /* last values were repetitive: print '^' and repetition count */
	    if (cval > 1) printf(" ^%d", cval);

	    /* print new value */
	    printf(" %d", tablearray[i].vals[j]);

	    /* remember new value */
	    lval = tablearray[i].vals[j];

	    /* re-initialize value count */
	    cval = 1;
	  }
	  else	/* value is same as last one, increment value count */
	    cval++;
	}
      }
      /* end of table: print last repetition index, if needed */
      if (cval > 1) printf(" ^%d", cval);
      printf("\n");
    }
  }
}
