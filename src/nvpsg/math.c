/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include "ACode32.h"
#include "acqparms.h"

extern void broadcastCodes(int, int, int*);

/*--------------------------------------------------------------
|	incr(a)
|	 a++;, where a is a real time variables
|
+---------------------------------------------------------------*/
void incr(int a)
{
    int code[2];
    code[0]=INC; code[1]=a;
    broadcastCodes(RTOP,2,code);
}
/*--------------------------------------------------------------
|	decr(a)
|	 a--;, where a is a real time variables
|
+---------------------------------------------------------------*/
void decr(int a)
{
    int code[2];
    code[0]=DEC; code[1]=a;
    broadcastCodes(RTOP,2,code);
}
/*--------------------------------------------------------------
|	assign(a,b)
|	assign value a to b, where a & b are real time variables
|
+---------------------------------------------------------------*/
void assign(int a, int b)
{
    int code[3];
    code[0]=SET; code[1]=a; code[2]=b;
    broadcastCodes(RT2OP,3,code);
}
/*--------------------------------------------------------------
|	dbl(a,b)
|	b = a * 2, where a & b are real time variables
|
+---------------------------------------------------------------*/
void dbl(int a, int b)
{
    int code[3];
    code[0]=DBL; code[1]=a; code[2]=b;
    broadcastCodes(RT2OP,3,code);
}
/*--------------------------------------------------------------
|	hlv(a,b)
|	b = a / 2, where a & b are real time variables
|
+---------------------------------------------------------------*/
void hlv(int a, int b)
{
    int code[3];
    code[0]=HLV; code[1]=a; code[2]=b;
    broadcastCodes(RT2OP,3,code);
}
/*--------------------------------------------------------------
|	modn(a,b,c)
|	c = a % b, where a,b & c are real time variables
|
+---------------------------------------------------------------*/
void modn(int a, int b, int c)
{
    int code[4];
    code[0]=MOD; code[1]=a; code[2]=b; code[3]=c;
    broadcastCodes(RT3OP,4,code);
}
/*--------------------------------------------------------------
|	mod4(a,b)
|	b = a % 4, where a & b are real time variables
|
+---------------------------------------------------------------*/
void mod4(int a, int b)
{
    int code[3];
    code[0]=MOD4; code[1]=a; code[2]=b; 
    broadcastCodes(RT2OP,3,code);
}
/*--------------------------------------------------------------
|	mod2(a,b)
|	b = a % 2, where a & b are real time variables
|
+---------------------------------------------------------------*/
void mod2(int a, int b)
{
    int code[3];
    code[0]=MOD2; code[1]=a; code[2]=b;
    broadcastCodes(RT2OP,3,code);
}
/*--------------------------------------------------------------
|	add(a,b,c)
|	c = a + b; where a & b are real time variables
|
+---------------------------------------------------------------*/
void add(int a, int b, int c)
{
    int code[4];
    code[0]=ADD; code[1]=a; code[2]=b; code[3]=c;
    broadcastCodes(RT3OP,4,code);
}
/*--------------------------------------------------------------
|	sub(a,b,c)
|	c = a - b; where a & b are real time variables
|
+---------------------------------------------------------------*/
void sub(int a, int b, int c)
{
    int code[4];
    code[0]=SUB; code[1]=a; code[2]=b; code[3]=c;
    broadcastCodes(RT3OP,4,code);
}
/*--------------------------------------------------------------
|	mult(a,b,c)
|	c = a * b; where a & b are real time variables
|
+---------------------------------------------------------------*/
void mult(int a, int b, int c)
{
    int code[4];
    code[0]=MUL; code[1]=a; code[2]=b; code[3]=c;
    broadcastCodes(RT3OP,4,code);
}
/*--------------------------------------------------------------
|	divn(a,b,c)
|	c = a / b; where a & b are real time variables
|
+---------------------------------------------------------------*/
void divn(int a, int b, int c)
{
    int code[4];
    code[0]=DIV; code[1]=a; code[2]=b; code[3]=c;
    broadcastCodes(RT3OP,4,code);
}
/*--------------------------------------------------------------
|	orr(a,b,c)
|	c = a | b; where a & b are real time variables
|
+---------------------------------------------------------------*/
void orr(int a, int b, int c)
{
    int code[4];
    code[0]=OR; code[1]=a; code[2]=b; code[3]=c;
    broadcastCodes(RT3OP,4,code);
}
