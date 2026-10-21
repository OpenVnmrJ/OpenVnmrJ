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
/* Pbox_err.h - Pbox error and memory allocation handler */

static char e_str[512];

void err(char *fstr) __attribute__((noreturn));
void flerr(char *fname) __attribute__((noreturn));
void pxerr(char *str) __attribute__((noreturn));

void err(char *fstr)
{
  printf("\n Pandora in panic! Can't find the \"%s\" file.\n", fstr);
  printf("Aborting...\n");
  exit(1);
}

void flerr(char *fname)
{
  printf("\n Pbox: Can't open file \"%s\" file.\n Aborting...\n", fname);
  exit(1);
}

void pxerr(char *str)
{
  printf("\n %s\n\n", str);
  exit(1);
}

void pxout(char *str, char dev)
{
  printf("%s", str);
}

double pxscan(void)
{
  double dbl;

  printf("\n %s\n\n", e_str);
  if (scanf("%lf", &dbl) != 1)
  {
    printf("Pbox: invalid numeric input.\n");
    return 0.0;
  }
  return dbl;
}


