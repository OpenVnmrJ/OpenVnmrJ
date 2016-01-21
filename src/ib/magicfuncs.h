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

#ifndef MAGICFUNCS_H
#define MAGICFUNCS_H

extern "C" {
    int loadAndExec(char *name);
    void magical_register_user_func_finder(int (*(*pfunc)(char *))(int , char **, int , char **));
    void magical_set_macro_search_path(char **dirs);
    void magical_set_error_print(void (*pfunc)(char *));
    void magical_set_info_print(void (*pfunc)(char *));
}

#endif /* MAGICFUNCS_H */


