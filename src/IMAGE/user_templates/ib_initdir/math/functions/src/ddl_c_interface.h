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

#ifndef DDL_C_INTERFACE_H
#define DDL_C_INTERFACE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif
    extern DDLSymbolTable *read_ddl_file(char *filename);
    extern DDLSymbolTable *create_ddl(int width, int height, int depth);
    extern DDLSymbolTable *clone_ddl(DDLSymbolTable *, int dataflag);
    extern void init_ddl(DDLSymbolTable *st, int w, int h, int d);
    extern float *get_ddl_data(DDLSymbolTable *st);
    extern unsigned char *get_uchar_data(DDLSymbolTable *,
			   double vmin, double vmax,
			   char **errmsg, unsigned char *buf);
    /*extern DDLSymbolTable *dup_ddl_data(DDLSymbolTable *st);*/
    extern void write_ddl_data(DDLSymbolTable *st, char *fname);
    extern int get_header_int(DDLSymbolTable *st, char *name, int *pval);
    extern int get_header_double(DDLSymbolTable *st, char *name, double *pval);
    extern int get_header_string(DDLSymbolTable *st, char *name, char **pval);
    extern int get_header_array_int(DDLSymbolTable *st, char *name, int n,
				    int *pval);
    extern int get_header_array_double(DDLSymbolTable *st, char *name, int n,
				double *pval);
    extern int get_header_array_string(DDLSymbolTable *st, char *name, int n,
				char **pval);
    extern int get_image_width(DDLSymbolTable *st);
    extern int get_image_height(DDLSymbolTable *st);
    extern int get_image_depth(DDLSymbolTable *st);
    extern double get_object_width(DDLSymbolTable *st);
    extern double get_object_height(DDLSymbolTable *st);
    extern double get_object_depth(DDLSymbolTable *st);
    extern int is_fdf_magic_number(char *filename, char *magic_string_list[]);
    extern double quiet_nan(long);
#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* DDL_C_INTERFACE_H */


