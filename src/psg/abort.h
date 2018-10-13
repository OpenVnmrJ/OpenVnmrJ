/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef ABORT_H
#define ABORT_H

extern void psg_abort(int error) __attribute__((noreturn));
extern void closeFiles();
extern void text_message(const char *format, ...) __attribute__((format(printf,1,2)));
extern void warn_message(const char *format, ...) __attribute__((format(printf,1,2)));
extern void abort_message(const char *format, ...) __attribute__((format(printf,1,2),noreturn));
extern void text_error(const char *format, ...) __attribute__((format(printf,1,2)));
extern void close_error(int success);
extern void setupPsgFile();
extern void putCmd(const char *format, ...) __attribute__((format(printf,1,2)));

#endif
