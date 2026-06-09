/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-----------------------------------------------------------------------
|  	sortnames/2
|	Performs a quicksort on the string pointer array
|			Author Greg Brissey  6/4/86
+------------------------------------------------------------------------*/
sortnames(item, count)		/* quick sort initial call */
char           *item[];
int             count;

{
   qs_strings(item, 0, count - 1);
}

/*-----------------------------------------------------------------------
|  qs_strings/3
|	recursively call main routine for quicksort of string pointer
|	array.
|			Author Greg Brissey   4/6/86
+------------------------------------------------------------------------*/
static
qs_strings(item, left, right)	/* quick sort */
char           *item[];
int             left;
int             right;

{
   register int    i,
                   j;
   register char  *x,
                  *y;

   i = left;
   j = right;
   x = item[(left + right) / 2];/* select mid points */

   do
   {
      while (strcmp(item[i], x) < 0 && i < right)
	 i++;
      while (strcmp(item[j], x) > 0 && j > left)
	 j--;

      if (i <= j)
      {
	 y = item[i];
	 item[i] = item[j];
	 item[j] = y;
	 i++;
	 j--;
      }
   } while (i <= j);

   if (left < j)
      qs_strings(item, left, j);
   if (i < right)
      qs_strings(item, i, right);
}

/*-----------------------------------------------------------------------
|  	sortset/2
|	Performs a quicksort on the string pointer array
|		carries along another array
|			Author Greg Brissey  6/4/86
+------------------------------------------------------------------------*/
sortset(item, item2, count)	/* quick sort initial call */
char           *item[];
char           *item2[];
int             count;

{
   qs_set(item, item2, 0, count - 1);
}

/*-----------------------------------------------------------------------
|  qs_set/4
|	recursively call main routine for quicksort of string pointer
|	array, with another array.
|			Author Greg Brissey   4/6/86
+------------------------------------------------------------------------*/
static
qs_set(item, item2, left, right)/* quick sort */
char           *item[];
char           *item2[];
int             left;
int             right;

{
   register int    i,
                   j;
   register char  *x,
                  *y;

   i = left;
   j = right;
   x = item[(left + right) / 2];/* select mid points */

   do
   {
      while (strcmp(item[i], x) < 0 && i < right)
	 i++;
      while (strcmp(item[j], x) > 0 && j > left)
	 j--;

      if (i <= j)
      {
	 y = item[i];
	 item[i] = item[j];
	 item[j] = y;
	 y = item2[i];
	 item2[i] = item2[j];
	 item2[j] = y;
	 i++;
	 j--;
      }
   } while (i <= j);

   if (left < j)
      qs_set(item, item2, left, j);
   if (i < right)
      qs_set(item, item2, i, right);
}
