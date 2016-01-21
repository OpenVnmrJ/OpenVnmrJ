/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>  // for sprintf definition  11/04/09   GMB

using namespace std;
#include "aipRQparser.h"
#include "aipVnmrFuncs.h"

RQparser *RQparser::rqparser = NULL;

RQparser::RQparser()
{
}

RQparser *RQparser::get()
{
    if(!rqparser) {
	rqparser = new RQparser();
    }
    return rqparser;
}

// a statement may consist of multiple sentences separated by ";" or "g".
// each element of the returned string list is for each sentence.
// the string contains 6 fileds, corresponding to selections of
// groups, images, slices, echoes, array, frames.
list<string>
RQparser::parseStatement(string str)
{
// group selection starts from "g" or "G", ends at \n or image or frame selection.
// image selection starts from "(", or "i", ends at \n or frame or next selection.
// frame selection starts with "[" or "f", ends at \n or next selection.
// slice selection starts with "s", ends at \n or next selection.
// echo selection starts with "e", ends at \n or next selection.
// array selection starts with "a", ends at \n or next selection.

// if not begin with "g" or "G", and "(" exists but is not first char, 
// assum substr before "(" is group selection.

    list<string> glist;
    if(str.length() <= 0) return glist;

    replaceVariables(str);
    toLowercase(str);

    int i = str.find("g", 0);

    string s;
    if(i != string::npos && i > 0) {
        int j = str.find("(", 0);
	if(j == string::npos) j = str.find("i", 0);
	if(j == string::npos) j = str.find("s", 0);
	if(j == string::npos) j = str.find("e", 0);
	if(j == string::npos) j = str.find("a", 0);
	if(j != string::npos && j > 0 && j < i) {
	   glist.push_back(parseSentence("g"+str.substr(0,i)));
	} else {
	   glist.push_back(parseSentence(str.substr(0,i)));
	} 
    } else if(i == string::npos) {
        int j = str.find("(", 0);
	if(j == string::npos) j = str.find("i", 0);
	if(j == string::npos) j = str.find("s", 0);
	if(j == string::npos) j = str.find("e", 0);
	if(j == string::npos) j = str.find("a", 0);
	if(j != string::npos && j > 0) {
	   glist.push_back(parseSentence("g"+str));
	} else {
	   glist.push_back(parseSentence(str));
	}
    } 

    int l;
    while(i != string::npos) {
	if((l = str.find("g", i+1)) == string::npos) {
	   l = str.length(); 
        }
	l -= i;
	if(l > 0) {
 	  glist.push_back(parseSentence(str.substr(i,l)));
	}
        i = str.find("g", i+1);
    }

    return glist;
}

string
RQparser::parseSentence(string str)
{
// this method parse a sentence and return a string of 6 fileds,
// corresponding to selections of groups, images, slices, array, echoes, frames.
// images cannot be selected at the same time as slices, array and echoes.
// need to specify groups to select slices,array and echoes.
// e.g., g1-s1-a2e1 (or gallsalla2e1) to select all slices, array 2, echo 1 of all groups.
// the return string is 1- NA 1- 2 1 NA, where NA means not images ans frames
// are not specified. "all" and "1-" both mean select all.

    if(str.length() <= 0) return "";
 
    string groups = "";
    string images = "";
    string frames = "";
    string slices = "";
    string array = "";
    string echoes = "";
    int i,s,e,a,f;

    if(str[0] == 'g') {
        
	i = str.find("(",1); 
	if(i == string::npos) i = str.find("i",1);
	if(i != string::npos) {
// select images
	    groups = str.substr(1,i-1);
            s = str.find("s",i);
	    if(s == string::npos) s = str.find("e",i);
	    if(s == string::npos) s = str.find("a",i);
	    f = str.find("[",i);
	    if(f == string::npos) f = str.find("f",i);
	    if(f != string::npos) {
	        if(s == string::npos) s = f;
	        images = str.substr(i+1,s-i-1);	
	    	if(str.length() > f+1)
		frames = str.substr(f+1,str.length()-f-1); 
	    } else if(s != string::npos) {
	        images = str.substr(i+1, s-i-1);
	    } else {
	     	if(str.length() > i+1)
	        images = str.substr(i+1, str.length()-i-1);
	    }
	} else if((s = str.find("s",1)) != string::npos) {
// select slice, array, echoes 
	    groups = str.substr(1,s-1);
	    if((a = str.find("a",s)) != string::npos) {
		slices = str.substr(s+1,a-s-1);
	   	if((e = str.find("e",a)) != string::npos) {
		   array = str.substr(a+1,e-a-1);
		   f  = str.find("[",e);
		   if(f == string::npos) f = str.find("f",e);
                   if(f != string::npos) {
		       echoes = str.substr(e+1,f-e-1);
		       if(str.length() > f+1)
                       frames = str.substr(f+1,str.length()-f-1);
		   } else {
		       if(str.length() > e+1)
                       echoes = str.substr(e+1, str.length()-e-1);
		   }
		} else {
		   f  = str.find("[",a);
                   if(f == string::npos) f = str.find("f",a);
                   if(f != string::npos) {
                       array = str.substr(a+1,f-a-1);
                       if(str.length() > f+1)
                       frames = str.substr(f+1,str.length()-f-1);
                   } else {
                       if(str.length() > a+1)
                       array = str.substr(a+1, str.length()-a-1);
                   }
		}
	     } else {
		 f  = str.find("[",s);
                 if(f == string::npos) f = str.find("f",s);
                 if(f != string::npos) {
                    slices = str.substr(s+1,f-s-1);
                    if(str.length() > f+1)
                    frames = str.substr(f+1,str.length()-f-1);
                 } else {
                    if(str.length() > s+1)
                    slices = str.substr(s+1, str.length()-s-1);
                 }
	     }
	} else if((a = str.find("a",1)) != string::npos) {
	    groups = str.substr(1,a-1);
	    if((e = str.find("e",a)) != string::npos) {
	        array = str.substr(a+1,e-a-1);
                f  = str.find("[",e);
                if(f == string::npos) f = str.find("f",e);
                if(f != string::npos) {
                   echoes = str.substr(e+1,f-e-1);
                   if(str.length() > f+1)
                   frames = str.substr(f+1,str.length()-f-1);
                } else {
                   if(str.length() > e+1)
                   echoes = str.substr(e+1, str.length()-e-1);
                }
	    } else {
                f  = str.find("[",a);
                if(f == string::npos) f = str.find("f",a);
                if(f != string::npos) {
                   array = str.substr(a+1,f-a-1);
                   if(str.length() > f+1)
                   frames = str.substr(f+1,str.length()-f-1);
                } else {
                   if(str.length() > a+1)
                   array = str.substr(a+1, str.length()-a-1);
                }
            }
	} else if((e = str.find("e",1)) != string::npos) {
            groups = str.substr(1,e-1);
            f  = str.find("[",e);
            if(f == string::npos) f = str.find("f",e);
            if(f != string::npos) {
               echoes = str.substr(e+1,f-e-1);
               if(str.length() > f+1)
               frames = str.substr(f+1,str.length()-f-1);
            } else {
               if(str.length() > e+1)
               echoes = str.substr(e+1, str.length()-e-1);
            }
	} else { 
	     f  = str.find("[",s);
             if(f == string::npos) f = str.find("f",s);
             if(f != string::npos) {
                groups = str.substr(1,f-1);
                if(str.length() > f+1)
                frames = str.substr(f+1,str.length()-f-1);
             } else {
                if(str.length() > 1)
                groups = str.substr(1, str.length()-1);
             }
	} 
    } else {
        f = str.find("[",0);
	if(f == string::npos) f = str.find("f",0);

	if(f != string::npos) {
	    if(f > 0)
	    images = str.substr(0,f);
	    if(str.length() > f+1)
	    frames = str.substr(f+1,str.length()-f-1); 
        } else if(str.length() > 0) {
            images = str;
        }
    }

    string newStr = "";
    if(groups.length() > 0) {
	trimStr(groups);
	newStr += groups + " ";
    } else newStr += "NA ";
    if(images.length() > 0) {
	trimStr(images);
	newStr += images + " ";
    } else newStr += "NA ";
    if(slices.length() > 0) {
	trimStr(slices);
	newStr += slices + " ";
    } else newStr += "NA ";
    if(array.length() > 0) {
	trimStr(array);
	newStr += array + " ";
    } else newStr += "NA ";
    if(echoes.length() > 0) {
	trimStr(echoes);
	newStr += echoes + " ";
    } else newStr += "NA ";
    if(frames.length() > 0) {
	trimStr(frames);
	newStr += frames;
    } else newStr += "NA";

    return newStr;
}


void
RQparser::replaceVariables(string &str)
{
    // replace ns, ne and na
// need to comment out getReal for this class to be stand alone.

    char value[MAXSTR];
    strcpy(value, "all");

    int i = str.find("ns", 0);
    while(i != string::npos) {
	sprintf(value, "%d", getReal("ns", 1));
	str.replace(i,2,value);
	i = str.find("ns",i+1);
    }

    i = str.find("ne", 0);
    while(i != string::npos) {
	sprintf(value, "%d", getReal("ne", 1));
	str.replace(i,2,value);
	i = str.find("ne",i+1);
    }

    // na = arraydim/nv
    i = str.find("na", 0);
    while(i != string::npos) {
        int arraydim = (int)getReal("arraydim", 1);
        int nv = (int)getReal("nv", 1);
	if(nv > 0 && arraydim >= nv) arraydim /= nv;
	sprintf(value, "%d", arraydim);
	str.replace(i,2,value);
	i = str.find("ne",i+1);
    }
}

void
RQparser::toLowercase(string &str)
{
    // replace G, S, E, A, C, R and F with lowercase

    int i = str.find("G", 0);
    while(i != string::npos) {
	str.replace(i,1,"g");
	i = str.find("G",i+1);
    }

    i = str.find("I", 0);
    while(i != string::npos) {
	str.replace(i,1,"i");
	i = str.find("I",i+1);
    }

    i = str.find("S", 0);
    while(i != string::npos) {
	str.replace(i,1,"s");
	i = str.find("S",i+1);
    }

    i = str.find("E", 0);
    while(i != string::npos) {
	str.replace(i,1,"e");
	i = str.find("E",i+1);
    }

    i = str.find("A", 0);
    while(i != string::npos) {
	str.replace(i,1,"a");
	i = str.find("A",i+1);
    }

    i = str.find("F", 0);
    while(i != string::npos) {
	str.replace(i,1,"f");
	i = str.find("F",i+1);
    }

    i = str.find("R", 0);
    while(i != string::npos) {
	str.replace(i,1,"r");
	i = str.find("R",i+1);
    }

    i = str.find("C", 0);
    while(i != string::npos) {
	str.replace(i,1,"c");
	i = str.find("C",i+1);
    }
}

void
RQparser::trimStr(string &str)
{
    int i = str.find(" ", 0);
    while(i != string::npos) {
	str.erase(i,1);
	i = str.find(" ",i);
    }

    i = str.find("(", 0);
    while(i != string::npos) {
	str.erase(i,1);
	i = str.find("(",i);
    }

    i = str.find(")", 0);
    while(i != string::npos) {
	str.erase(i,1);
	i = str.find(")",i);
    }

    i = str.find("[", 0);
    while(i != string::npos) {
	str.erase(i,1);
	i = str.find("[",i);
    }

    i = str.find("]", 0);
    while(i != string::npos) {
	str.erase(i,1);
	i = str.find("]",i);
    }

    i = str.find(";", 0);
    while(i != string::npos) {
	str.erase(i,1);
	i = str.find(";",i);
    }

    i = str.find("\n", 0);
    while(i != string::npos) {
	str.erase(i,1);
	i = str.find("\n",i);
    }

    i = str.find("\t", 0);
    while(i != string::npos) {
	str.erase(i,1);
	i = str.find("\t",i);
    }
}

list<int>
RQparser::parseSelections(string str)
{
    list<int> slist; 
    if(str.length() <=0) return slist;
  
    int i = 0;
    int l;
    while(i != string::npos && i < str.length()) {
	if((l = str.find(",", i+1)) == string::npos) {
           l = str.length();
        }
        l -= i;
        if(l > 0) {
	   list<int> slist2 = parseSelection(str.substr(i,l));
	   //slist.merge(slist2);
	   list<int>::iterator nitr;
           for(nitr = slist2.begin(); nitr != slist2.end(); ++nitr) {
              slist.push_back(*nitr);
           }
	}
	i = str.find(",",i+1);
	if(i != string::npos) i++;
    }
    return slist;
}

list<int>
RQparser::parseSelection(string str)
{
    list<int> slist; 
    if(str.length() <=0) return slist;
  
    string s;
    int step = -1;
    int i = str.find(":", 0);
    if (i != string::npos) {
	i++;
	int l = str.length() - i; 
	if(l > 0) {
	   s = str.substr(i,l);
	   step = replaceMath(s);
	} else step = 1;
    } else {
        i = str.length()+1; 
	step = 1;
    }

    str = str.substr(0,i-1);
    int lower = -1;
    int upper = -1;
    i = str.find("-", 0);
    if (i != string::npos) {
        if(i > 0) {
	  s = str.substr(0,i);
	  lower = replaceMath(s);
	} else lower = 1;
	i++;
	int l = str.length() - i;
	if(l > 0) {
	   s = str.substr(i,l);
	   upper = replaceMath(s);
	} else upper = 1;
    } else {
	lower = replaceMath(str);
	upper = lower;
    }

    if(step == 0) step = 1;
    if(lower > upper) step = -step;
    for(i=lower; (step>0 && i<=upper) || (step<0 && i>=upper); i+=step) {
	slist.push_back(i);
    }
    return slist;
}

list<int>
RQparser::parseGroups(string str, int ng)
{
    list<int> slist; 

    if(str.length() <=0) return slist;
    if(strcasecmp(str.c_str(), "NA") == 0) return slist;

    if(strcasecmp(str.c_str(), "all") == 0) {
	for(int i=0; i<ng; i++) slist.push_back(i+1);
    } else {
	int i = str.find("-", 0); 
	if(i != string::npos) {
	  int j = str.find(":", 0); 
	  if(j != string::npos && j>i && (j-i)==1) {
	     char cs[MAXSTR];
	     sprintf(cs, "%d",ng);
	     str.insert(j, cs);
	  } else if(j == string::npos && i == str.length()-1) { 
	     char cs[MAXSTR];
	     sprintf(cs, "%d",ng);
	     str += cs;
	  }
	}
	slist = parseSelections(str);
    }
    return slist;
}

list<int>
RQparser::parseImages(string str, int ni)
{
    list<int> slist; 

    if(str.length() <=0) return slist;
    if(strcasecmp(str.c_str(), "NA") == 0) return slist;

    if(strcasecmp(str.c_str(), "all") == 0) {
	for(int i=0; i<ni; i++) slist.push_back(i+1);
    } else {
	int i = str.find("-", 0); 
	if(i != string::npos) {
	  int j = str.find(":", 0); 
	  if(j != string::npos && j>i && (j-i)==1) {
	     char cs[MAXSTR];
	     sprintf(cs, "%d",ni);
	     str.insert(j, cs);
	  } else if(j == string::npos && i == str.length()-1) { 
	     char cs[MAXSTR];
	     sprintf(cs, "%d",ni);
	     str += cs;
	  }
	}
	slist = parseSelections(str);
    }
    return slist;
}

list<int>
RQparser::parseSlices(string str, int ni)
{
// slices, array and echoes are used only if images is "NA",
// this method is different from parseImages by always return
// all if selection is not specified.
    list<int> slist; 

    if(str.length() <=0 ||
	strcasecmp(str.c_str(), "NA") == 0 ||
	strcasecmp(str.c_str(), "all") == 0) {
	for(int i=0; i<ni; i++) slist.push_back(i+1);
    } else {
	int i = str.find("-", 0); 
	if(i != string::npos) {
	  int j = str.find(":", 0); 
	  if(j != string::npos && j>i && (j-i)==1) {
	     char cs[MAXSTR];
	     sprintf(cs, "%d",ni);
	     str.insert(j, cs);
	  } else if(j == string::npos && i == str.length()-1) { 
	     char cs[MAXSTR];
	     sprintf(cs, "%d",ni);
	     str += cs;
	  }
	}
	slist = parseSelections(str);
    }
    return slist;
}

list<int>
RQparser::parseFrames(string str, int rows, int cols, int ni)
{
// assume layout is up to date.
// ni is number of images, n is number of frames.

    list<int> slist; 
    if(str.length() <= 0 || strcasecmp(str.c_str(), "NA") == 0) { 
        str = "1";
    }

    int n = rows*cols;

    int r = str.find("r", 0);
    int c = str.find("c", 0);
    if(r != string::npos) {
      r++;
      int l;
      list<int> il;
      list<int>::iterator itr;
      while(r != string::npos && r < str.length()) {
        if((l = str.find("r", r+1)) == string::npos) {
           l = str.length();
        }
        l -= r;

        if(l > 0) {
	  il = parseSelections(str.substr(r,l));
	  for (itr = il.begin(); itr != il.end(); ++itr) {
	     for(int j=1; j<=cols; j++) {
		slist.push_back(((*itr)-1)*cols + j);
	     }
	  }
        }
        r = str.find("r", r+1);
	if(r != string::npos) r++;
      }
    } else if(c != string::npos) {
      c++;
      int l;
      list<int> il;
      list<int>::iterator itr;
      while(c != string::npos && c < str.length()) {
        if((l = str.find("c", c+1)) == string::npos) {
           l = str.length();
        }
        l -= c;

        if(l > 0) {
	  il = parseSelections(str.substr(c,l));
	  for (itr = il.begin(); itr != il.end(); ++itr) {
	     for(int j=1; j<=rows; j++) {
		slist.push_back((j-1)*cols + (*itr));
	     }
	  }
        }
        c = str.find(" ", c+1);
	if(c != string::npos) c++;
      }
    } else if(strcasecmp(str.c_str(), "all") == 0) {
	for(int i=0; i<n; i++) slist.push_back(i+1);
    } else {
	int i = str.find("-", 0); 
	if(i != string::npos) {
	  int first = atoi(str.substr(0,i).c_str()) - 1;
	  int j = str.find(":", 0); 
	  if(j != string::npos && j>i && (j-i)==1) {
	     char cs[MAXSTR];
	     sprintf(cs, "%d",ni + first);
	     //sprintf(cs, "%d",n);
	     str.insert(j, cs);
	  } else if(j == string::npos && i == str.length()-1) { 
	     char cs[MAXSTR];
	     sprintf(cs, "%d",ni + first);
	     //sprintf(cs, "%d",n);
	     str += cs;
	  }
	}
	slist = parseSelections(str);
    }
    
    return slist;
}

int
RQparser::replaceMath(string str)
{
   if(str.length() < 1) return 0;
  
   int i = 0;
   int l;
   int v1 = 0;
   while(i != string::npos && i < str.length()) {
	if((l = str.find("-", i+1)) == string::npos) {
           l = str.length();
        }
        l -= i;
 
	if(l > 0) {
           string s1 = str.substr(i,l);
	   int j = 0;
           int v2 = 0;
	   while(j != string::npos && j < s1.length()) {
	      if((l = s1.find("+", j+1)) == string::npos) {
                  l = s1.length();
              }
              l -= j;

              if(l > 0) {
		string s2 = s1.substr(j,l);
                int k = 0;
		int v3 = 0;
                while(k != string::npos && k < s2.length()) {
                   if((l = s2.find("/", k+1)) == string::npos) {
                       l = s2.length();
                   }
                   l -= k;

                   if(l > 0) {
		      string s3 = s2.substr(k,l);
                      int m = 0;
		      int v4 = 1;
                      while(m != string::npos && m < s3.length()) {
                         if((l = s3.find("*", m+1)) == string::npos) {
                             l = s3.length();
                         }
                         l -= m;

                         if(l > 0) {
                            v4 *= atoi(s3.substr(m,l).c_str()); 
                         } 
	                 m = s3.find("*",m+1);
	                 if(m != string::npos) m++;
	              }
		      if(v3 != 0 && v4 != 0) v3 /= v4;
		      else v3 = v4;
                   } 
	           k = str.find("/",k+1);
	           if(k != string::npos) k++;
	        }
		v2 += v3;
	      }
	      j = str.find("+",j+1);
	      if(j != string::npos) j++;
	   }
	   if(v1 != 0) v1 -= v2;
	   else v1 = v2;
	}
	i = str.find("-",i+1);
	if(i != string::npos) i++;
   }
   return v1; 
}
