/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
/* 
 */

#include <stdio.h>
#include <string.h>

#include "vsfunc.h"

#ifndef TRUE
#define TRUE (0 == 0)
#define FALSE (!TRUE)
#endif

VsTable::VsTable(int init_size)
{
    ref_count = 1;
    size = init_size;
    table = new u_char[size];
    /*fprintf(stderr,"table[%d] allocated\n", size);/*CMP*/
}

VsTable::~VsTable()
{
    delete[] table;
    /*fprintf(stderr,"table[%d] deleted\n", size);/*CMP*/
}

VsFunc::VsFunc(int init_size)
{
    if (init_size){
	lookup = new VsTable(init_size);
    }else{
	lookup = NULL;
    }
    command = NULL;
}

VsFunc::VsFunc(VsFunc *old)
{
    // Copy the old data into the new instance, but use the same lookup
    // table, incrementing the ref_count.
    *this = *old;
    if (lookup){
	lookup->ref_count++;
	/*fprintf(stderr,"VsFunc(VsFunc *): table=0x%x, ref_count=%d\n",
		lookup->table, lookup->ref_count);/*CMP*/
    }
    if (old->command){
	command = new char[strlen(old->command) + 1];
	strcpy(command, old->command);
    }
}

VsFunc::~VsFunc()
{
    if (lookup){
	/*fprintf(stderr,"~VsFunc(): table=0x%x, ref_count=%d\n",
		lookup->table, lookup->ref_count-1);/*CMP*/
    }
    if (lookup && --lookup->ref_count == 0){
	delete lookup;
    }
    delete[] command;
}

int
VsFunc::isequal(VsFunc *other)
{
    if (min_data != other->min_data
	|| max_data != other->max_data
	|| uflow_cmi != other->uflow_cmi
	|| oflow_cmi != other->oflow_cmi
	|| lookup != other->lookup)
    {
	return FALSE;
    }else{
	return TRUE;
    }
}
