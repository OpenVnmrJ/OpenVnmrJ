/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "aipRQstudy.h"
#include "aipReviewQueue.h"
#include "aipVnmrFuncs.h"

RQstudy::RQstudy() : RQnode()
{
    initStudy();
}

RQstudy::RQstudy(string str) : RQnode()
{
    if(str.find("filenode",0) != string::npos) { // str is xml 
        readNode(str);
    } else if(str.find("/",0) == 0) { // str is key or path
 	initStudy(str);	
    } else {
 	initStudy();	
    }
}

void
RQstudy::initStudy(string str)
{
    string dir = "";
    string name = "";
    string ext = "";
    string copy = "";

    if(str.find_last_of("/") == str.length()-1)
      str = str.substr(0,str.length()-1);

    if(str.find("/",0) == 0) { // str is key or path.

      size_t i1;
      if((i1 = str.find_last_of(" ")) != string::npos) {
	copy = str.substr(i1);
	str = str.substr(0, i1); 
      } else copy = ReviewQueue::get()->getNextCopy(STUDY, str);


      if((i1 = str.find(" ",1)) == string::npos)
         str.replace(str.find_last_of("/"), 1," ");

      if((i1 = str.find(" ",1)) != string::npos) {
         dir = str.substr(0,i1);
         if(dir.find_last_of("/") == dir.length()-1)
           dir = dir.substr(0,dir.length()-1);
         name = str.substr(i1+1);
      }

      if((i1 = name.find(".")) != string::npos) {
         ext = name.substr(i1, name.length()-i1);
         name = name.substr(0,i1);
      }
    }

    setAttribute("type","study");
    setAttribute("dir",dir);
    setAttribute("name",name);
    setAttribute("ext",ext);
    setAttribute("copy",copy);
    setAttribute("expand","yes");
    setAttribute("display","no");
    setAttribute("nid","");
}
