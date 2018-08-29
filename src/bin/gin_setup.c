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

/*  Program for Tek42xx series which creates arrow icon
    in segment 1000 for use with the mouse accessory.	*/

main()
{
	printf( "\033%%!0" );		/*  Need TEK mode for the following */

	printf( "\033SV" );		/*  Set all future segments  */
	encodeTekInt( -2 );		/*  to be invisible (briefly) */
	encodeTekInt( 0 );

	printf( "\033SP" );		/*  Set pivot point to 1000,1000 */
	coordinate(1000,1000);

	printf( "\033ML" );		/*  Set current color to white */
	encodeTekInt( 1 );		/*  for the marker */

	printf( "\033SK" );		/*  Delete segment 1000 */
	encodeTekInt( 1000 );

	printf( "\033SO" );		/*  Open segment 1000 */
	encodeTekInt( 1000 );

	printf( "\033LF" );
	coordinate( 0, 0 );
	printf( "\033LF" );
	coordinate( 1000, 1000 );
	printf( "\033LG" );
	coordinate( 1030, 1000 );
	printf( "\033LF" );
	coordinate( 1060, 940 );
	printf( "\033LG" );
	coordinate( 1000, 1000 );
	printf( "\033LG" );
	coordinate( 1000, 970 );
	printf( "\033LF" );
	coordinate( 0, 0 );

	printf( "\033SC" );		/*  Close current segment */

	printf( "\033SP" );
	coordinate( 0, 0 );

	printf( "\033SV" );		/*  Set all future segments */
	encodeTekInt( -2 );		/*  to be visible (permanent) */
	encodeTekInt( 1 );

	printf( "\033NT" );		/*  No EOL character in GIN report */

	printf( "\033IC" );		/*  Specify segment 1000 */
	encodeTekInt( 64 );		/*  to be used with the */
	encodeTekInt( 1000 );		/*  mouse GIN device */

	printf( "\033%%!1" );		/*  Back to ANSI mode */
}

static encodeTekInt( ival )
{
	int	i1, i2, i3, jval;

	jval = (ival < 0) ? -ival : ival;
	i1 = (jval >> 10) + 64;
	i2 = ((jval >> 4) % 64) + 64;
	i3 = jval % 16 + 32;
	if ( ival >= 0 ) i3 += 16;

	if ( i1 != 64 ) printf( "%c", i1 );
	if ( i2 != 64 ) printf( "%c", i2 );
	printf( "%c", i3 );
}

coordinate(x,y)
int x,y;
{
	int	a, b, c, d, e;

	a=0x20+((y>>7)&0x1f);		/* y_high */
	e=((y&3)<<2)+(x&3)+0x60;	/* extra  */
	b=0x60+((y>>2)&0x1f);		/* y_low  */
	c=0x20+((x>>7)&0x1f);		/* x_high */
	d=0x40+((x>>2)&0x1f);		/* x_low  */

	printf("%c%c%c%c%c",a,e,b,c,d);
}
