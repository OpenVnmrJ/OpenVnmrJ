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
#include <stdio.h>
/*
 * landscape - send a file to the HP LaserJet in landscape mode
 * lands10 - send a file to the HP LaserJet in landscape mode (10 pitch)
 * portrait - send a file to the HP LaserJet in portrait mode
 *	1) expand tabs to equivalent spaces
 *	2) change NL to CR/NL
 *	3) send start and reset escape sequences
 *	4) send form feed after each input file
 *	5) option (-v) to change the vertical spaciing
 *	6) option (-f) to fit 66 lines on a page  (not compatible with -v)
 *	7) option (-s) to place 2 66-line pages side by side in landscape
 *		mode ("2-up")
 *
 * The name with which the program is invoked determines the orientation.
 *
 * These 3 fonts are the only fixed width fonts available in the B
 * cartridge; to use the proportional fonts, a smarter program (such as
 * troff) is needed.
 *
 * History
 * 3/6/85 David Brown Varian: based on expand (from Berkeley)
 */


#define LANDSCAPE "\033(8U\033&l1O\033(s0p16.66h8.5v0s-1b0T"
#define LANDS10 "\033(8U\033&l1O\033(s0p10h12v0s0b3T"
#define PORTRAIT "\033(8U\033&l0O\033(s0p10h12v0s0b3T"
#define TIMES10 "\033&10O\033(0U\033(s1p10v0s0b5T"
#define TIMES10x "\033(0U\033(s1p10vtB"
#define FIT66 "\033&l66P\033&l2E\033&l7.6C\033&l66F"
/*#define LFIT66 "\033&l66P\033&l2E\033&l5.4545C\033&l66F"*/
#define LFIT66 "\033&l66P\033&l5.4545C\033&l66F"
#define RESET "\033E"
#define COLUMN "\033&a%dC"
#define ROW "\033&a%dR"
#define SET_LEFT_MARGIN "\033&a%dL"
#define COLWIDTH 88
#define NL '\n'
#define CR '\r'
#define FF '\f'
#define VERT 'v'
#define MAXVERT 126
#define MINVERT 0
#define VERTICAL "\033&l%.4fC"
#define FIT 'f'
#define UP2 's'
#define MAXLINES 66

char	obuf[BUFSIZ];
int	nstops;
int	tabstops[100];

int	landscape = 0;
int	up2 = 0;

double atof();

main(argc, argv)
	int argc;
	char *argv[];
{
	register int c, column;
	register int n;
	int line;

	setbuf(stdout, obuf);

/* send start sequence for HP Laserjet	*/
	fputs(RESET,stdout);
	if (strcmp(argv[0],"landscape") == 0) {
		fputs(LANDSCAPE,stdout);
		landscape = 1;
	}
	else
		if (strcmp(argv[0],"lands10") == 0) {
			fputs(LANDS10,stdout);
			landscape = 1;
		}
		else
			if (strcmp(argv[0],"times10") == 0) {
				fputs(TIMES10,stdout);
				landscape = 0;
			}

			else { 				/* anything else */
				fputs(PORTRAIT,stdout);
				landscape = 0;
			}
		
	
	argc--, argv++;

	do {
		while (argc > 0 && argv[0][0] == '-') {
			getstops(argv[0]);
			argc--, argv++;
		}
		if (argc > 0) {
			if (freopen(argv[0], "r", stdin) == NULL) {
				perror(argv[0]);
				exit(1);
			}
			argc--, argv++;
		}

		column = 0;
		line = 0;
		for (;;) {
			c = getc(stdin);
			if (c == -1)
				break;
			switch (c) {

			case '\t':
				if (nstops == 0) {
					do {
						putchar(' ');
						column++;
					} while (column & 07);
					continue;
				}
				if (nstops == 1) {
					do {
						putchar(' ');
						column++;
					} while (((column - 1) % tabstops[0])
						!= (tabstops[0] - 1));
					continue;
				}
				for (n = 0; n < nstops; n++)
					if (tabstops[n] > column)
						break;
				if (n == nstops) {
					putchar(' ');
					column++;
					continue;
				}
				while (column < tabstops[n]) {
					putchar(' ');
					column++;
				}
				continue;

			case '\b':
				if (column)
					column--;
				putchar('\b');
				continue;

			default:
				putchar(c);
				column++;
				continue;

			case '\n':
				line++;
				if ((up2) && (line == MAXLINES)) {
					line = 0;
					nextcol();
				}
				else {
					putchar(CR);	/* carriage return */
					putchar(c);
				}
				column = 0;
				continue;
			case FF:
				line = 0;
				if (up2)
					nextcol();
				else
					putchar(c);
				continue;
			}
		}
		if (up2)
			nextcol();
/*		else
			putchar(FF);		/* form feed	*/
	} while (argc > 0);
	fputs(RESET,stdout);
	exit(0);
}

getstops(cp)
	register char *cp;
{
	register int i;
	
	cp++;
	if (*cp == VERT) {
		double vertical;
		cp++;
		vertical = atof(cp);
		if ((int) vertical > MAXVERT  || (int) vertical < MINVERT) {
			fprintf(stderr,"Bad vertical index: %f\n",vertical);
			exit(1);
		}
		printf(VERTICAL,vertical);
		return(0);
	}

	if (*cp == FIT) {
		cp++;
		if (landscape) {
			/*fprintf(stderr,"landscape = %d\n",landscape);*/
			fputs(LFIT66,stdout);
		}
		else
			fputs(FIT66,stdout);
		return(0);
	}

	if (*cp == UP2) {
		cp++;
		fputs(LFIT66,stdout);
		up2 = 1;
		return(0);
	}

	nstops = 0;
	for (;;) {
		i = 0;
		while (*cp >= '0' && *cp <= '9')
			i = i * 10 + *cp++ - '0';
		if (i <= 0 || i > 256) {
bad:
			fprintf(stderr, "Bad tab stop spec\n");
			exit(1);
		}
		if (nstops > 0 && i <= tabstops[nstops-1])
			goto bad;
		tabstops[nstops++] = i;
		if (*cp == 0)
			break;
		if (*cp++ != ',')
			goto bad;
	}
}

nextcol() {
	static int field = 0;

	if (field == 0) {
		field = 1;
		movcol(COLWIDTH);
		movrow(0);
		setlmargin(COLWIDTH);
	}
	else {
		field = 0;
		movcol(0);
		movrow(0);
		setlmargin(0);
		putchar(FF);
	}
}

movcol(col)
int col;
{
	printf(COLUMN,col);
}

movrow(row)
int row;
{
	printf(ROW,row);
}

setlmargin(col)
int col;
{
	printf(SET_LEFT_MARGIN,col);
}
