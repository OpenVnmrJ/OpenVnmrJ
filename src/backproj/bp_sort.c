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
/****************************************************************/
/*								*/
/*	authors:	Martin Staemmler			*/
/*			Peter Barth				*/
/*								*/
/*	Institute for biomedical Engineering (IBMT)		*/
/*	D - 66386 St. Ingbert, Germany				*/
/*	Ensheimerstrasse 48					*/
/*	Tel.: (+49) 6894 980251					*/
/*	Fax:  (+49) 6894 980400					*/
/*								*/
/****************************************************************/
/*								*/
/*	date:		15.03.94				*/
/*	revision:	initial release				*/
/*								*/
/****************************************************************/
#include	"data.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	"bpvalues.h"
#define		MAX_SIZE	512
#define		ERROR		-1
#define		TRUE		1
#define     FALSE 0

main (argc,argv)
int	argc;
char	*argv[];
{
char		filein[128], fileout[128];
char		*data;
int		i, n_prof, n_echo;
struct	datafilehead	fhp;
struct	datablockhead	bhp;

if (argc != 5) {
    printf ("usage: filein fileout #echo # projection\n");
    }
else {
    strcpy (filein, argv[1]);
    strcpy (fileout, argv[2]);
    n_echo = atoi (argv[3]);
    n_prof = atoi (argv[4]);
    }
 if (access_file (filein, &fhp, &bhp, TRUE) == ERROR) {
    fprintf (stderr,"BP_SORT: can't access file\n");
    return (ERROR);
    }
if (create_file (fileout, &fhp, &bhp) == ERROR) {
    fprintf (stderr,"BP_SORT: can't create file\n");
    return (ERROR);
    }
if ((data = (char *)calloc(n_prof * MAX_SIZE, sizeof(int))) == NULL) {
    fprintf (stderr,"BP_SORT: can't allocate data space\n");
    return (ERROR);
    }
for (i=0; i<n_echo; i++) {
    if (read_vnmr_data (filein, &fhp, i,
                     n_echo*n_prof, n_echo,
                     (char *)data, TRUE) == ERROR){
        fprintf (stderr,"BP_SORT: can't read to profile buffer\n");
        return (ERROR);
        }
     if (write_vnmr_data (fileout, &fhp, i * n_prof, 
                      (i+1) * n_prof, 1,
                      (char *)data) == ERROR) {
        fprintf (stderr,"BP_SORT: can't write shuffled profiles\n");
        return (ERROR);
        }
    }   /* end of loop for all n_echo */
free (data);
}


