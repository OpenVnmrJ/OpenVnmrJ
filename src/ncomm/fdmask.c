/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <sys/time.h>

int is_fdmask_clear( register fd_set *fdmaskp )
{
   register int iter;

   for (iter = 0; iter < FD_SETSIZE; iter++) {
      if (FD_ISSET(iter, fdmaskp) )
         return( 0 );
   }
   return( 1 );
}

/*  Return the largest file descriptor + 1 in the mask of file descriptors.  */
int find_maxfd( register fd_set *fdmaskp )
{
   register int iter;

   for (iter = FD_SETSIZE - 1; iter >= 0; iter--) {
      if (FD_ISSET(iter, fdmaskp))
         return(iter+1);
   }
   return( 0 );
}
