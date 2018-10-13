/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef _MOVIE_H
#define _MOVIE_H
/************************************************************************
*									
*  @(#)movie.h 18.1 03/21/08 (c)1992 SISCO
*
*************************************************************************/

class Movie_frame
{
 public: 
  
  char *filename ;
  Imginfo *img ;
  class Movie_frame *nextframe, *prevframe;

  Movie_frame() 
  {
      filename = NULL;
      img = NULL;
  }

  Movie_frame(char *fname) 
  {
      filename = new char[strlen(fname) + 1];
      strcpy(filename, fname);
      img = NULL;
  }

  ~Movie_frame() 
  {
      if (filename){
	  delete filename;
      }
      detach_imginfo(img);
  }

};

Movie_frame *
in_a_movie(Imginfo *);

extern Movie_frame *movie_head;

#endif /* _MOVIE_H */
