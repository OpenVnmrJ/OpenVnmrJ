/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  Definitions used for Labelling Axes and Parameters */

/*  Identifiers For the 6 Label Fields */

#define FIELD1	1
#define FIELD2	2
#define FIELD3	3
#define FIELD4	4
#define FIELD5	5
#define FIELD6	6

/*  Calculate the starting column of Field a */

#define COLUMN(a)  (((a) - 1) * 12) + 2

/*  Parameter name identifiers for display in the parameter fields     */
/*  By using,  for example,  CR_NAME,  an application program does not */
/*  need to know if the actual label is "cr", "cr1", or "cr2".         */
/*  BLANK_NAME is used to display blanks in the field (i.e. clear the  */
/*  field.  NO_NAME is used to leave the field alone.                  */

#define BLANK_NAME	0
#define NO_NAME		10
#define CR_NAME		20
#define DELTA_NAME	30
#define LP_NAME		40
#define RP_NAME		50
#define SP_NAME		60
#define WP_NAME		70
#define SW_NAME		80

#define NOSHOW	0
#define SHOW	1

/*  Flag for scaling frequency for KHz,  ppm,  cm,  etc  */

#define NOT_SCALED	0
#define SCALED		1

/*  Identifier for Horizontal and Vertical Data Axis              */
/*  The Frequency Domains and / or Time Domains are Mapped to the */
/*  Horizontal and Vertical direction                             */

#define DIRECTION_ERROR	       -1
#define HORIZ			1
#define VERT			2

/*  Identifier for NORMAL and REVERSED Data Axis                  */
/*  These definitions are used to decide which axis corresponds   */
/*  to rows and columns of a 2D data set.  The "row" direction    */
/*  is referred to as NORDIR and the column direction is REVDIR   */
/*  If the REVDIR is HORIZ,  then revflag is TRUE.                */
/*  For 2D data sets,  f2 is NORMDIR and f1 is REVDIR             */
/*  For 2D planes of 3D data sets:                                */
/*  f1f2 plane,  f2 is NORMDIR and f1 is REVDIR                   */
/*  f1f3 plane,  f3 is NORMDIR and f1 is REVDIR                   */
/*  f2f3 plane,  f3 is NORMDIR and f2 is REVDIR                   */

#define NORMDIR			0
#define REVDIR			1

/*  Frequency domain  */

#define FN0_DIM			0
#define FN1_DIM			2
#define FN2_DIM			4
#define FN3_DIM			6

/*  Time domain  */

#define SW0_DIM			1
#define SW1_DIM			3
#define SW2_DIM			5

/*   label type NOUNIT:                :  used for dscale and pscale  */
/*   label type UNIT1 :      ppm (sc)  :  used for dscale and pscale  */
/*   label type UNIT2 :     (ppm)(sc)  :  used for vertical 2d axis   */
/*   label type UNIT3 :     (ppm (sc)) :  used for horizontal 2d axis */
/*   label type UNIT4 :     (ppm)      :  used for parameter label fields */

#define NOUNIT			0
#define UNIT1			1
#define UNIT2			2
#define UNIT3			3
#define UNIT4			4

#define ONE_D			1
#define TWO_D			2
#define THREE_D			3
#define FOUR_D			4
