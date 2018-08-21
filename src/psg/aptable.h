/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*************************
*  Constant Definitions  *
*************************/

#define MAXTABLE	60
#define MAXPATHL	128
#define BASEINDEX	1024
#define TADD		0	/* table addition operation */
#define TSUB		1	/* table subtraction operation */
#define TMULT		2	/* table multiplication operation */
#define TDIV		3	/* table division operation */


/**************************
*  Structure Definitions  *
**************************/

struct _Tableinfo
{
   int		reset;
   int          table_number;
   int          *table_pointer;
   int		*hold_pointer;
   int          table_size;
   int         	divn_factor;
   int          auto_inc;
   int		wrflag;
   codeint	acodeloc;
   codeint     	indexptr;
   codeint     	destptr;
};

typedef struct _Tableinfo	Tableinfo;


/***********************************
*  External Variable Declarations  *
***********************************/

extern char	curexp[MAXPATHL],	/* current expt directory */
		userdir[MAXPATHL],	/* user directory */
		systemdir[MAXPATHL];	/* system directory */

extern codeint	t1, t2, t3, t4, t5, t6,
		t7, t8, t9, t10, t11, t12,
		t13, t14, t15, t16, t17, t18,
		t19, t20, t21, t22, t23, t24,
		t25, t26, t27, t28, t29, t30,
		t31, t32, t33, t34, t35, t36,
		t37, t38, t39, t40, t41, t42,
		t43, t44, t45, t46, t47, t48,
		t49, t50, t51, t52, t53, t54,
		t55, t56, t57, t58, t59, t60;
		
extern int	table_order[MAXTABLE],
		tmptable_order[MAXTABLE],
		loadtablecall,
		last_table;

extern Tableinfo	*Table[MAXTABLE];
