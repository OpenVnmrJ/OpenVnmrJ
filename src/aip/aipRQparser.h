/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPRQPARSER_H
#define AIPRQPARSER_H

#include <string>
#include <list>

class RQparser
{

public:

   RQparser();
   static RQparser *get();
   list<string> parseStatement(string str);
   string parseSentence(string str);
   void replaceVariables(string &str);
   void toLowercase(string &str);
   void trimStr(string &str);
   list<int> parseSelections(string str);
   list<int> parseSelection(string str);
   list<int> parseGroups(string str, int ng);
   list<int> parseImages(string str, int ni);
   list<int> parseSlices(string str, int ni);
   list<int> parseFrames(string str, int rows, int cols, int ni);
   int replaceMath(string str);

private:

   static RQparser *rqparser;

};

#endif /* AIPRQPARSER_H */
