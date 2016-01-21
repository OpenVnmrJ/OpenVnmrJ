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
/* fileops.c - used with tap_main.c to assemble 'tape' */
#define  DIRECT_ID	1
#define  DATA_ID	2
#define  PROG_ID	3
#define  ARRAY_ID	4

int nscmp( s1, s2 )
char *s1, *s2;
{
	char		c1, c2;
	int		i1, i2, l1, l2;

/*  Preliminaries:  Establish length of each string.  If both strings are
    null, this counts as a match.  If the first string is null and the
    second is not, this is not a match.  If the second string is null and
    the first one is "*", this counts as a match; otherwise, a null second
    string cannot match.

    When all of this is complete, we konw both strings are not null.	*/

	l1 = strlen( s1 );
	l2 = strlen( s2 );

	if (l1 <= 0 && l2 <= 0) return( 1 );
	if (l1 <= 0)		return( 0 );
	if (l2 <= 0)
	 if (strcmp( s1, "*" ) == 0) return( 1 );
	 else			     return( 0 );

	i1 = 0;
	i2 = 0;
	do {
		c1 = *(s1+i1);
		c2 = *(s2+i2);

/*  Following check may be superfluous, but I feel better if it is done.  */

		if ( c1 == '\0' && c2 == '\0' ) return( 1 );

/*  Eliminate distinction, upper-case vs. lowercase.  */

		if ( c1 >= 'A' && c1 <= 'Z') c1 |= 0x20;
		if ( c2 >= 'A' && c2 <= 'Z') c2 |= 0x20;

/*  Check for 'placeholder' in first string.  */

		if (c1 == '%' || c1 == '#') {
			if (c2 == '\0') return( 0 );
			i1++;
			i2++;
		}

/*  Now check for wildcard in first string  */

		else if ( c1 == '*' ) {
			i1++;
			if (i1 >= l1) return( 1 );
			while ( l2-i2 > 0 )
			 if (nscmp( s1+i1, s2+i2 )) return( 1 );
			 else i2++;
			return( 0 );
		}
		else if (c1 != c2)
		 return( 0 );
		else {
			i1++;
			i2++;
		}
	} while (i1 <= l1 && i2 <= l2);

	return( 1 );
}

/*  New feature -  arrays are supported.  Extension is ".annn" where
    "nnn" is the number of entries in the array (the I2 value). 	*/

make_ext( ext_ptr, i1_val, i2_val )
char *ext_ptr;
int i1_val, i2_val;
{
	*ext_ptr = '\000';
	if (i1_val == PROG_ID)
	 strcat( ext_ptr, ".prg" );
	else if (i1_val == DATA_ID)
	 strcat( ext_ptr, ".dat" );
	else if (i1_val == ARRAY_ID) {
		if (i2_val < 1) return;
		sprintf( ext_ptr, ".a%d", i2_val );
	}
	else {
		if (i2_val < 1) return;
		sprintf( ext_ptr, ".%d", i2_val );
	}
}
