/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdlib.h>

#include "acodes.h"
#include "rfconst.h"
#include "acqparms.h"

/*--------------------------------------------------------------
|				Author Greg Brissey  6/29/86
+---------------------------------------------------------------*/
/*--------------------------------------------------------------
|	incr(a)
|	 a++;, where a is a real time variables
|
+---------------------------------------------------------------*/
incr(a)
codeint a;
{
    notinhwloop("incr");
    putcode(INCRFUNC);
    putcode(a);
}
/*--------------------------------------------------------------
|	decr(a)
|	 a--;, where a is a real time variables
|
+---------------------------------------------------------------*/
decr(a)
codeint a;
{
    notinhwloop("decr");
    putcode(DECRFUNC);
    putcode(a);
}
/*--------------------------------------------------------------
|	assign(a,b)
|	assign value a to b, where a & b are real time variables
|
+---------------------------------------------------------------*/
assign(a,b)
codeint a;
codeint b;
{
    notinhwloop("assign");
    putcode(ASSIGNFUNC);
    putcode(a);
    putcode(b);
}
/*--------------------------------------------------------------
|	dbl(a,b)
|	b = a * 2, where a & b are real time variables
|
+---------------------------------------------------------------*/
dbl(a,b)
codeint a;
codeint b;
{
    notinhwloop("dbl");
    putcode(DBLFUNC);
    putcode(a);
    putcode(b);
}
/*--------------------------------------------------------------
|	hlv(a,b)
|	b = a / 2, where a & b are real time variables
|
+---------------------------------------------------------------*/
hlv(a,b)
codeint a;
codeint b;
{
    notinhwloop("hvl");
    putcode(HLVFUNC);
    putcode(a);
    putcode(b);
}
/*--------------------------------------------------------------
|	modn(a,b,c)
|	c = a % b, where a,b & c are real time variables
|
+---------------------------------------------------------------*/
modn(a,b,c)
codeint a;
codeint b;
codeint c;
{
    notinhwloop("modn");
    putcode(MODFUNC);
    putcode(a);
    putcode(b);
    putcode(c);
}
/*--------------------------------------------------------------
|	mod4(a,b)
|	b = a % 4, where a & b are real time variables
|
+---------------------------------------------------------------*/
mod4(a,b)
codeint a;
codeint b;
{
    notinhwloop("mod4");
    putcode(MOD4FUNC);
    putcode(a);
    putcode(b);
}
/*--------------------------------------------------------------
|	mod2(a,b)
|	b = a % 2, where a & b are real time variables
|
+---------------------------------------------------------------*/
mod2(a,b)
codeint a;
codeint b;
{
    notinhwloop("mod2");
    putcode(MOD2FUNC);
    putcode(a);
    putcode(b);
}
/*--------------------------------------------------------------
|	add(a,b,c)
|	c = a + b; where a & b are real time variables
|
+---------------------------------------------------------------*/
add(a,b,c)
codeint a;
codeint b;
codeint c;
{
    notinhwloop("add");
    putcode(ADDFUNC);
    putcode(a);
    putcode(b);
    putcode(c);
}
/*--------------------------------------------------------------
|	sub(a,b,c)
|	c = a - b; where a & b are real time variables
|
+---------------------------------------------------------------*/
sub(a,b,c)
codeint a;
codeint b;
codeint c;
{
    notinhwloop("sub");
    putcode(SUBFUNC);
    putcode(a);
    putcode(b);
    putcode(c);
}
/*--------------------------------------------------------------
|	mult(a,b,c)
|	c = a * b; where a & b are real time variables
|
+---------------------------------------------------------------*/
mult(a,b,c)
codeint a;
codeint b;
codeint c;
{
    notinhwloop("mult");
    putcode(MULFUNC);
    putcode(a);
    putcode(b);
    putcode(c);
}
/*--------------------------------------------------------------
|	divn(a,b,c)
|	c = a / b; where a & b are real time variables
|
+---------------------------------------------------------------*/
divn(a,b,c)
codeint a;
codeint b;
codeint c;
{
    notinhwloop("divn");
    putcode(DIVFUNC);
    putcode(a);
    putcode(b);
    putcode(c);
}
/*--------------------------------------------------------------
|	orr(a,b,c)
|	c = a | b; where a & b are real time variables
|
+---------------------------------------------------------------*/
orr(a,b,c)
codeint a;
codeint b;
codeint c;
{
    notinhwloop("orr");
    putcode(ORFUNC);
    putcode(a);
    putcode(b);
    putcode(c);
}
/*--------------------------------------------------------------
|	ifnz(a,b,c)
|	if a - b != 0 jump to c; where a & b are real time variables
|	c if an offset to location to jump to.
|
+---------------------------------------------------------------*/
ifnz(a,b,c)
codeint a;
codeint b;
codeint c;
{
    notinhwloop("ifnz");
    putcode(IFNZFUNC);
    putcode(a);
    putcode(b);
    putcode(c);
}
/*--------------------------------------------------------------
|	ifmi(a,b,c)
|	if a - b < 0 jump to c; where a & b are real time variables
|	c if an offset to location to jump to.
|
+---------------------------------------------------------------*/
ifmi(a,b,c)
codeint a;
codeint b;
codeint c;
{
    notinhwloop("ifmi");
    putcode(IFMIFUNC);
    putcode(a);
    putcode(b);
    putcode(c);
}
