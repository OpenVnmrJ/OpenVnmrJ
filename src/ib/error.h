/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifdef HEADER_ID


#else /* (not) HEADER_ID */

#ifndef ERROR_H_DEFINED
#define ERROR_H_DEFINED

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)

int  sys_error (char *message, ...);
void error (char *message, ...);
void fatal (int exit_code, char *message, ...);
void error_exit (int exit_code);

#else /* not __STDC__ && not __cplusplus && not c_plusplus */

int  sys_error( /* va_alist */ );
void error( /* va_alist */ );
void fatal( /* int exit_code, va_alist */ );
void error_exit( /* int exit_code */ );

#endif

#ifdef DEBUG

#define PRINTF(args) printf args
#define FPRINTF(args) fprintf args
#define ERROR(args) error args
#define SYS_ERROR(args) sys_error args
#define FATAL(args) fatal args
#define ERROR_EXIT(args) error_exit args

#else

#define PRINTF(args)
#define FPRINTF(args)
#define ERROR(args)
#define SYS_ERROR(args)
#define FATAL(args)
#define ERROR_EXIT(args)

#endif

#endif /* (not) ERROR_H_DEFINED */

#endif /* HEADER_ID */
