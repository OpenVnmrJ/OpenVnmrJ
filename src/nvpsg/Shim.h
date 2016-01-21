/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef INC_SHIMS_H
#define INC_SHIMS_H


#define MAX_SHIMS 48
#define MAX_SHIMSET 16
#define NUM_SHIMSET  MAX_SHIMSET+1

class Shim
{

   private:
	char		**sh_names;
	int 		  shimset;
	void		  init_shimnames_by_setnum(int newshimset);
	int		  isactive(int index);
        int		  codes[MAX_SHIMS+10];

   public:
	Shim();
	char		 *get_shimname(int index);
	int		  get_shimindex(char *shimname);
	int		  init_shimnames(int tree);
	int		 *loadshims();

};

#endif
