/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*------------------------------------------------------------
|
|   This module helps emulate the Mouse for those terminals
|   which lack a hardware mouse device.
|
+-------------------------------------------------------------*/

#include  "vnmrsys.h"
#include  <stdio.h>
#include  <unistd.h>
#ifdef UNIX
#include  <signal.h>
#include  <sys/time.h>
#endif 
#include  "wjunk.h"

#define  CTL_A		1
#define  ESC		0x1b
#define  GRAF		0x1d
#define  icon_incre	5
#define  min_iconx	10
#define  min_icony	10
#define  iabs( x )	(((x) < 0) ? -(x) : (x))

static char	icon_buf[ 514 ];
static char	*drawIconPtr;
static int	cur_iconx = 0;			/*  New coordinates */
static int	cur_icony = 0;
static int	iconx;				/*  Current coordinates */
static int	icony;
static int	max_iconx;
static int	max_icony;
static int	icon_active = 0;
static int	sa, sb, sc, sd, se;
static int	ix, iy;

int		icon_moving = 0;

extern int	active;				/*  wjunk.c  */
extern int	hasMouse;			/*  wjunk.c  */
extern int      query();
extern int GisXORmode();


static void icon_normalmode();
static void icon_xormode();
static void idraw(int x, int y);
static void imove(int x, int y);
#ifndef VNMRJ
static void coord0(int x, int y);
#endif
int ms_sleep(int interval );

/*  The maximum limits of the Icon are set to 10 pixel units
    from the corresponding edge of the display screen.		*/

void setIconLimits()
{
#ifndef VNMRJ
	if (Wistek41xx())
        {
	  if (Wistek4x05()) {
		max_iconx = 469;
		max_icony = 359;
	  }
	  else {				/*  Must be 4107 */
		max_iconx = 629;
		max_icony = 479;
	  }
        }
#endif
}

/*  Draws a arrow  */

static void draw_icon(int x, int y )
{
	if ( !icon_active ) return;

	imove( x+5, y );
	idraw( x, y );
	imove( x, y-1 );
	idraw( x, y-5 );
	imove( x+10, y-10 );
	idraw( x+1, y-1 );
}

/*  Does NOT draw icon, merely defines its position  */

void Ginit_icon(int  x, int y )
{
	cur_iconx = iconx = x;
	cur_icony = icony = y;
	icon_active = 0;
	sa = sb = sc = sd = se = 0;
}

/*  Draw the icon for the first time.  */

void Gactivate_icon()
{
	int	cur_window, was_normal;

	icon_active = 131071;
	icon_moving = 0;

	if ( (cur_window = active) != 2 )
	  Wgmode();
	drawIconPtr = &icon_buf[ 0 ];
	if ( (was_normal = !GisXORmode()) )
	 icon_xormode();
	draw_icon( iconx, icony );
	if( was_normal)
	  icon_normalmode();
	*drawIconPtr = '\0';
#ifdef UNIX
	printf( "%s", &icon_buf[ 0 ] );
#else 
	vms_outputchars( &icon_buf[ 0 ], drawIconPtr - &icon_buf[ 0 ] );
#endif 
	if (cur_window != 2)
	  Wsetactive( cur_window );
}

/*  Move the icon from one point to another.  */

void Gmove_icon()
{
	int	cur_window, was_normal;

	if ( !icon_active ) return;
	if ( (cur_window = active) != 2 )
	  Wgmode();
	drawIconPtr = &icon_buf[ 0 ];
	if ( (was_normal = !GisXORmode()) )
	  icon_xormode();
	draw_icon( iconx, icony );			/* Erase old */
	*drawIconPtr = '\0';
#ifdef UNIX
	printf( "%s", &icon_buf[ 0 ] );
#else 
	vms_outputchars( &icon_buf[ 0 ], drawIconPtr - &icon_buf[ 0 ] );
#endif 
	drawIconPtr = &icon_buf[ 0 ];
	draw_icon( cur_iconx, cur_icony );		/* Draw new */
	if( was_normal)
	  icon_normalmode();
	*drawIconPtr = '\0';
#ifdef UNIX
	printf( "%s", &icon_buf[ 0 ] );
#else 
	vms_outputchars( &icon_buf[ 0 ], drawIconPtr - &icon_buf[ 0 ] );
#endif 
	if (cur_window != 2)
	  Wsetactive( cur_window );

	iconx = cur_iconx;
	icony = cur_icony;
	icon_moving = 0;
}

/*  Erase the icon.  */

void Gdeactive_icon()
{
	int		cur_window, was_normal;

	if ( !icon_active ) return;
	if ( (cur_window = active) != 2 )
	  Wgmode();
	drawIconPtr = &icon_buf[ 0 ];
	if ( (was_normal = !GisXORmode()) )
	  icon_xormode();
	draw_icon( iconx, icony );			/* Erase old */
	if( was_normal )
	  icon_normalmode();
	*drawIconPtr = '\0';
#ifdef UNIX
	printf( "%s", &icon_buf[ 0 ] );
#else 
	vms_outputchars( &icon_buf[ 0 ], drawIconPtr - &icon_buf[ 0 ] );
#endif 
	if (cur_window != 2)
	  Wsetactive( cur_window );

	icon_active = 0;
	icon_moving = 0;
}

static void icon_normalmode()
{
#ifndef VNMRJ
    if (Wisgraphon() || Wishds()) {
	*(drawIconPtr++) = ESC;
	*(drawIconPtr++) = CTL_A;
/*      Gprintf("%c",ESC);	*/
    }
    else if (Wistek()) {
	*(drawIconPtr++) = ESC;
	*(drawIconPtr++) = 'R';
	*(drawIconPtr++) = 'U';
	*(drawIconPtr++) = '!';
	*(drawIconPtr++) = ';';
	*(drawIconPtr++) = '6';
/*      Gprintf("%cRU!;6",ESC);   */
    }
#endif
}

static void icon_xormode()
{
#ifndef VNMRJ
    if (Wisgraphon() || Wishds()) {
	*(drawIconPtr++) = ESC;
	*(drawIconPtr++) = '\025';
/*      Gprintf("%c\025",ESC);   */
    }
    else if (Wistek()) {
	*(drawIconPtr++) = ESC;
	*(drawIconPtr++) = 'R';
	*(drawIconPtr++) = 'U';
	*(drawIconPtr++) = '!';
	*(drawIconPtr++) = '7';
	*(drawIconPtr++) = '6';
/*      Gprintf("%cRU!76",ESC);  */
    }
#endif
}

/*  Separate routine used for drawing the icon because
    1.  You have to call setdisplay() to effect graphics normally.
    2.  Issues of shifting above the parameter display region or
        of scaling should be avoided.				*/

static void idraw(int x, int y)
{
#ifdef VNMRJ
  (void) x;
  (void) y;
#else
  int		incre, iter, limit, new_x, new_y, old_x, old_y, use_x;

    if (Wisgraphon() || Wishds()) {
	ix = x; iy = y;
	coord0(ix,iy);
    }
    else if (Wistek()) {

/*  Take the easy way out, if possible.  */

	if (x-ix == 0 || y-iy == 0) {
		*(drawIconPtr++) =  ESC;
		*(drawIconPtr++) =  'R';
		*(drawIconPtr++) =  'R';
/*		Gprintf( "%cRR", ESC);  */
		coord0( ix, iy );
		coord0( x, y );
		*(drawIconPtr++) = '1';
	}

/*  Have to 'rasterize' the vector into a series of
    vertical or horizontal vectors.			*/

	else {
		use_x = iabs( x-ix ) < iabs( y-iy );
		if (use_x) {
			limit = iabs( x-ix ) + 1;
			incre = ( x < ix ) ? -1 : 1;
		}
		else {
			limit = iabs( y-iy ) + 1;
			incre = ( y < iy ) ? -1 : 1;
		}

		old_x = ix;
		old_y = iy;

/*  For each pixel in the shorter direction...  */

		for (iter = 0; iter < limit; iter++) {
			if (use_x) {
				new_x = old_x;
				new_y = (y-iy) * (iter+1) / limit + iy;
			}
			else {
				new_x = (x-ix) * (iter+1) / limit + ix;
				new_y = old_y;
			}
			*(drawIconPtr++) =  ESC;
			*(drawIconPtr++) =  'R';
			*(drawIconPtr++) =  'R';
			coord0( old_x, old_y );
			coord0( new_x, new_y );
			*(drawIconPtr++) = '1';
			if (use_x) {
				old_x += incre;
				old_y = new_y;
			}
			else {
				old_x = new_x;
				old_y += incre;
			}
		}
	}

/*  Update internal position values.  */

	ix = x;
	iy = y;
    }
#endif
}

static void imove(int x, int y)
{
    ix = x;
    iy = y;
#ifndef VNMRJ
    if (Wisgraphon() || Wishds()) {
	*(drawIconPtr++) = GRAF;
/*	Gprintf("%c",GRAF);  */
        sa=sb=sc=sd=0;		/* force all 4 chars to be send */
        coord0(x,y);	/* move to current position */
    }
    else if (Wistek()) {
        sa=sb=sc=sd=se=0;	/* force all 5 chars to be sent */
    }
#endif
}

#ifndef VNMRJ
static void coord0(int x, int y)
{
    int	yhigh,ylow,xhigh,xlow,extra;

/*  GraphOn, HDS accept Tek 4010 style 10 bit codes, with a
    maximum of 4 characters per x, y position.		*/

    if (Wisgraphon() || Wishds())
    {
	yhigh=0x20+((y>>5)&0x1f);	/* y-high */
	ylow=0x60+(y&0x1f);		/* y_low  */
	xhigh=0x20+((x>>5)&0x1f);	/* x-high */
	xlow=0x40+(x&0x1f);		/* x-low  */

    /* determine most efficient method of going to coordinate */

	if (sa!=yhigh) {
	    if (sc!=xhigh) {
		*(drawIconPtr++) = yhigh;
		*(drawIconPtr++) = ylow;
		*(drawIconPtr++) = xhigh;
		*(drawIconPtr++) = xlow;
/*		Gprintf("%c%c%c%c",yhigh,ylow,xhigh,xlow);  */
	    }
            else if(sb!=ylow) {
		*(drawIconPtr++) = yhigh;
		*(drawIconPtr++) = ylow;
		*(drawIconPtr++) = xlow;
/*		Gprintf("%c%c%c",yhigh,ylow,xlow);  */
	    }
            else {
		*(drawIconPtr++) = yhigh;
		*(drawIconPtr++) = xlow;
/*		Gprintf("%c%c",yhigh,xlow);   */
	    }
	}
	else {
	    if(sc!=xhigh) {
		*(drawIconPtr++) = ylow;
		*(drawIconPtr++) = xhigh;
		*(drawIconPtr++) = xlow;
/*		Gprintf("%c%c%c",ylow,xhigh,xlow);  */
	    }
	    else if(sb!=ylow) {
		*(drawIconPtr++) = ylow;
		*(drawIconPtr++) = xlow;
/*		Gprintf("%c%c",ylow,xlow);  */
	    }
	    else {
		*(drawIconPtr++) = xlow;
/*		Gprintf("%c",xlow);  */
	    }
	}
	sa=yhigh; sb=ylow; sc=xhigh; sd=xlow;		/* save last values */
    }

/*  Tektronix terminals can accept 12 bit codes, with a
    maximum of 5 characters per x, y position.		*/

    else if (Wistek()) {
	yhigh=0x20+((y>>7)&0x1f);		/* y_high */
	extra=((y&3)<<2)+(x&3)+0x60;		/* extra  */
	ylow=0x60+((y>>2)&0x1f);		/* y_low  */
	xhigh=0x20+((x>>7)&0x1f);		/* x_high */
	xlow=0x40+((x>>2)&0x1f);		/* x_low  */

	if (sa!=yhigh) {
	   if (sc!=xhigh) {
		*(drawIconPtr++) = yhigh;
		*(drawIconPtr++) = extra;
		*(drawIconPtr++) = ylow;
		*(drawIconPtr++) = xhigh;
		*(drawIconPtr++) = xlow;
/*		Gprintf( "%c%c%c%c%c", yhigh, extra, ylow, xhigh, xlow );  */
	   }
	   else {
		*(drawIconPtr++) = yhigh;
		*(drawIconPtr++) = extra;
		*(drawIconPtr++) = ylow;
		*(drawIconPtr++) = xlow;
/*		Gprintf( "%c%c%c%c", yhigh, extra, ylow, xlow );  */
	   }
	}
	else {
	   if (sc!=xhigh) {
		*(drawIconPtr++) = extra;
		*(drawIconPtr++) = ylow;
		*(drawIconPtr++) = xhigh;
		*(drawIconPtr++) = xlow;
/*		Gprintf( "%c%c%c%c", extra, ylow, xhigh, xlow );  */
	   }
	   else {
		*(drawIconPtr++) = extra;
		*(drawIconPtr++) = ylow;
		*(drawIconPtr++) = xlow;
/*		Gprintf( "%c%c%c", extra, ylow, xlow );  */
	   }
	}
	sa=yhigh; sb=ylow; sc=xhigh; sd=xlow; se=extra;
    }
}
#endif

/* argument gives direction  */
void shiftIconPos( int d )
{

	if (d < 0 || 3 < d) return;		/*  Bad direction */

/*  If the Icon had not been moving, or if there are no
    characters waiting to be read, pause for 200 milliseconds.  */

	if ( !icon_moving ) ms_sleep( 200 );
	else if ( !query() ) ms_sleep( 200 );
	icon_moving = 131071;

	if (d == 0) cur_icony += icon_incre;
	else if (d == 1) cur_icony -= icon_incre;
	else if (d == 2) cur_iconx += icon_incre;
	else cur_iconx -= icon_incre;

/*  Keep position of icon within bounds.  */

	if (cur_iconx > max_iconx) cur_iconx = max_iconx;
	else if (cur_iconx < min_iconx) cur_iconx = min_iconx;
	if (cur_icony > max_icony) cur_icony = max_icony;
	else if (cur_icony < min_icony) cur_icony = min_icony;
}

/*  Following routine required for UNIX version of ms_sleep().
    It serves as the interrupt routine when the interval timer
    expires.  In this situation, it can simply return.		*/

#ifdef UNIX
static void
sigalarm_irpt()
{

/*  No requirement for this interrupt routine to reregister
    itself; it is not used with an interval timer.              */

	return;
}
#endif 

/*  This routine causes the process to pause for the given number
    of milliseconds.  Designed to support intervals of up to 2
    billion milliseconds, or nearly 3 weeks.			*/

int ms_sleep(int interval )
{
#ifdef UNIX
	int			ival, sec, msec;
	sigset_t		qmask;
	struct itimerval	tval;
	struct sigaction	sigalarm_vec;

	if (interval < 0) return( -1 );
 
	sigemptyset( &qmask );
	sigalarm_vec.sa_handler = sigalarm_irpt;
	sigalarm_vec.sa_mask    = qmask;
	sigalarm_vec.sa_flags   = 0;
	ival = sigaction( SIGALRM, &sigalarm_vec, NULL );
	if (ival != 0) {
		perror( "sigaction failure" );
		return( -1 );
	}

	tval.it_interval.tv_sec  = 0;
	tval.it_interval.tv_usec = 0;
	sec  = interval / 1000;
	msec = interval % 1000;
	tval.it_value.tv_sec  = sec;
	tval.it_value.tv_usec = msec*1000;
	ival = setitimer( ITIMER_REAL, &tval, NULL );
	if (ival != 0) {
		perror( "set timer failure" );
		return( -1 );
	}

/*  Following call suspends this process until something
    happens, here the interval timer expiring.			*/

	ival = pause();
	return( 0 );
#else 

	char		wait_time[ 20 ];
	int		bin_time[ 2 ], wt_descr[ 2 ];
	int		centi_secs, days, hours, minutes, seconds;

/*  VMS offers time resolution only to hundredths of a second
    (10 milliseconds or 1 centisecond)				*/

	interval = interval / 10;
	centi_secs = interval % 100;
	seconds    = interval / 100;
	minutes    = seconds / 60;
	seconds    = seconds % 60;
	hours      = minutes / 60;
	minutes    = minutes % 60;
	days       = hours / 24;
	hours      = hours % 24;
	
	sprintf( &wait_time[ 0 ], "%d %d:%d:%d.%d",
			days,
			hours,
			minutes,
			seconds,
			centi_secs
	);
	wt_descr[ 0 ] = strlen( &wait_time[ 0 ] );
	wt_descr[ 1 ] = (int) &wait_time[ 0 ];

	SYS$BINTIM( &wt_descr[ 0 ], &bin_time[ 0 ] );
	SYS$SCHDWK( 0, 0, &bin_time[ 0 ], 0 );
	SYS$HIBER();
#endif 
}

/*  Used by terminal.c to determine where the cursor is
    when the user presses a simulated mouse button.	*/

int getSimulCursorPos(int *xptr, int *yptr )
{
	if (hasMouse) ABORT;
	if (!icon_active) ABORT;

	*xptr = iconx;
	*yptr = icony;
	RETURN;
}
