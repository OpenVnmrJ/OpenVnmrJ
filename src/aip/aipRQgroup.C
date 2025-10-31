/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

using namespace std;

#include "aipRQgroup.h"
#include "aipReviewQueue.h"
#include "aipRQparser.h"
#include "aipDataManager.h"
#include "aipGframeManager.h"
#include "aipDataInfo.h"
#include "aipVnmrFuncs.h"

using namespace aip;

RQgroup::RQgroup() : RQnode()
{
    initGroup();
}

RQgroup::RQgroup(string str, int firstFrame, bool show) : RQnode()
{
    initGroup();

    if(str.find("filenode",0) != string::npos) { // str is xml
        // make a group from group attributes in xml format
        readNode(str);

    } else if(str.find("/",0) == 0) { // str is key or path
      int orient = ReviewQueue::get()->getPlaneOrient(str);
      if(orient != -1) {
	make3Dgroup(str);
	return;
      }
      // make a group from an image key (the first image of a group).
      initGroup(str, show);

      // make children only if copy is 0
      // otherwise children will be added later.
      size_t p = str.find_last_of(" ");
      if(p != string::npos && str.substr(p+1) == "0") {
        string path = getPath();
        makeChildren(path.c_str());
/*
	makeMaps();

	// some fdf header does not have correct array_index.
        // this will screw up sortting.
        if(dimMap.size() != indexMap.size()) {
	  // warning message
	  //ib_errmsg("RQ: fdf header does not have correct array_index.\n");
        }
*/   
        // some fdf header does not have correct array_dim.
        // this can be fixed since children.size() is the total size. 
/*
        int i = atoi(getAttribute("ns").c_str());
        i *= atoi(getAttribute("ne").c_str());
        if(i > 0) i = getChildrenCount()/i;
        if(i != atoi(getAttribute("na").c_str())) {
         setAttribute("na", i);
	 // warning message
	 //ib_errmsg("RQ: fdf header does not have correct array_dim.\n");
        }
*/
      } 

      // this is done here instead of in initGroup because 
      // na is correct by now.
      initSelection(firstFrame);

    } else return; 

}

void
RQgroup::initGroup()
{
    setParent((RQnode *)NULL);
    setAttribute("type","scan");
    setAttribute("dir","");
    setAttribute("name","");
    setAttribute("ext","");
    setAttribute("copy","");
    setAttribute("group","");
    setAttribute("ns","");
    setAttribute("na","");
    setAttribute("ne","");
    setAttribute("images","");
    setAttribute("slices","");
    setAttribute("array","");
    setAttribute("echoes","");
    setAttribute("frames","");
    setAttribute("nid","");
    setAttribute("display","");
    setAttribute("sort","eas");
    setAttribute("expand","no");
    setAttribute("rank",2);
    setAttribute("matrix","0 0 0");

    imageList.clear();

    m_dataInfo = (spDataInfo_t)NULL;

// tsize put a cap on the max images entries to write out to
// a xml file, and subsequently shown in RQ table.
   int tsize = (int)getReal("rqImageNodes", 0);
   setAttribute("tsize",tsize);
}

void
RQgroup::make3Dgroup(string str)
{
    if(str.find(" ", 0) != string::npos) {
	//str is image key
	str = ReviewQueue::get()->getGroupPath(str);
    }

    //m_dataInfo = dataInfo3D;
    setAttribute("rank","3");
    setAttribute("type","scan");
    setDirAndName(str);
    setAttribute("ext","");   
    setAttribute("copy"," 0");   
    setAttribute("ns","1");
    setAttribute("na","1");
    setAttribute("ne","1");
    setAttribute("images","1-");
    setAttribute("slices","1-");
    setAttribute("array","1");
    setAttribute("echoes","1");
    setAttribute("frames","1-");
}

void
RQgroup::initGroup(string str, bool show)
{
// str may be image key or path, or group key or path.

    string path;
    string key;
    string copy = "";

    if(str.find_last_of("/") == str.length()-1)
      str = str.substr(0,str.length()-1);

    setDirAndName(str);

    size_t i1;
    if((i1 = str.find(".fid")) != string::npos) {
      return;
    }

    if((i1 = str.find(".fdf")) != string::npos) {
	// str is image path or key
	i1 = i1 +4;
 	if(i1 == (str.length()-1)) path = str;
	else path = str.substr(0, i1);

	if((i1 = path.find(" ",1)) == string::npos) 
           path.replace(path.find_last_of("/"), 1," ");
	key = path + " 0";
	path = path.substr(0,path.find_last_of(" "));
    } else {
	// str is group path or key 
	key = "";
	path = str;
	if((i1 = path.find_last_of(" ")) != string::npos) 
	   path = path.substr(0,i1);
	if((i1 = path.find_first_of(" ")) != string::npos)
           path.replace(path.find_first_of(" "), 1,"/");
    } 
	 
    if((i1 = str.find_last_of(" ")) != string::npos) {
	copy = str.substr(i1);
    } else copy = ReviewQueue::get()->getNextCopy(SCAN, path);

   // get the following 3 attributes and group parameters
   int slices=1;
   int echoes=1;
   int array_dim=1;
   int rank = 2;
   int matrix[3] = {0, 0, 0};

    spDataInfo_t dataInfo = DataManager::get()->getDataInfoByKey(key);
    if(dataInfo != (spDataInfo_t)NULL) {
 
      m_dataInfo = dataInfo;
      if(!dataInfo->st->GetValue("rank", rank)) rank = 2;
      if(rank == 2) {
	if(!dataInfo->st->GetValue("matrix", matrix[0], 0)) matrix[0] = 0;
	if(!dataInfo->st->GetValue("matrix", matrix[1], 1)) matrix[1] = 0;
	matrix[2] = 0;
        if( !dataInfo->st->GetValue("slices", slices)
          || !dataInfo->st->GetValue("echoes", echoes)
          || !dataInfo->st->GetValue("array_dim", array_dim)) {
	  getDimFromProcpar(path, &slices, &array_dim, &echoes);
        }
      } else {
	if(!dataInfo->st->GetValue("matrix", matrix[0], 0)) matrix[0] = 0;
	if(!dataInfo->st->GetValue("matrix", matrix[1], 1)) matrix[1] = 0;
	if(!dataInfo->st->GetValue("matrix", matrix[2], 2)) matrix[2] = 0;
	slices = matrix[0] + matrix[1] + matrix[2]/2;
	echoes = 1; 
	array_dim = 1;
      }
      setAttribute("rank", rank); 
      char str[MAXSTR];
      sprintf(str, "%d %d %d", matrix[0], matrix[1], matrix[2]);
      setAttribute("matrix", str); 
    } else {
	 getDimFromProcpar(path, &slices, &array_dim, &echoes);
    }
    string display = "no";
    if(show) display = "yes";

    int sort = (int)getReal("rqsort2",1);
    if(sort == 0) setAttribute("sort", "no");
    else if(sort == 1) setAttribute("sort", "eas");
    else if(sort == 2) setAttribute("sort", "sea");
    else setAttribute("sort", "eas");
    setAttribute("type","scan");
    setAttribute("ext","");   
    setAttribute("copy",copy);   
    setAttribute("ns",slices);   
    setAttribute("na",array_dim);   
    setAttribute("ne",echoes);   
    setAttribute("display",display);   
    setAttribute("expend","no");   
    setAttribute("group","");
    setAttribute("nid","");
}

void
RQgroup::initSelection(int firstFrame)
{
    char c[MAXSTR];

   string imageSelection;
   string frameSelection;
   string sliceSelection;
   string echoSelection;
   string arraySelection;
   int slices = atoi(getAttribute("ns").c_str());
   int array_dim = atoi(getAttribute("na").c_str());
   int echoes = atoi(getAttribute("ne").c_str());

    // get recondisplay
    double recondisplay = 1;
    string par = "recondisplay";
    getRealFromProcpar(getPath(), &par, &recondisplay, 1);
    int step = (int)recondisplay;
    if(step <= 0) step = 1;
    
    int gsize = getGroupSize();
/*
    int tsize = atoi(getAttribute("tsize").c_str());
    // get max number of images to display
    int max = 1000;
    if(tsize > 0) max = tsize; 
    if((gsize/step) > max) { 
       sprintf(c, "1-%d", max); 
    } else if(gsize > 0) {
       sprintf(c, "1-%d:%d",gsize, step); 
    } else {
       sprintf(c, "1-:%d", step); 
    }
*/
    sprintf(c, "1-%d:%d",gsize, step); 
    imageSelection = c;
    sprintf(c, "1-%d",slices); 
    sliceSelection = c;
    sprintf(c, "1-%d",echoes); 
    echoSelection = c;
    sprintf(c, "1-%d",array_dim); 
    arraySelection = c;
    sprintf(c, "%d-",firstFrame); 
    frameSelection = c;

    setAttribute("images",imageSelection);   
    setAttribute("slices",sliceSelection);   
    setAttribute("array",arraySelection);   
    setAttribute("echoes",echoSelection);   
    setAttribute("frames",frameSelection);   
}

bool RQgroup::canSort(string str) {
   if(!doSort || str == "" || str == "no" || getAttribute("rank") == "3" ||
        getChildren()->size() != (size_t) getScanSize()) return false;
   else return true; 
}

void
RQgroup::makeMaps()
{
   dimMap.clear(); 
   indexMap.clear(); 
   list<RQnode>::iterator itr;
   ChildList *l = getChildren();
   RQimage *image;
   string sort = getAttribute("sort");
   if(canSort(sort)) {
    for(itr=l->begin(); itr != l->end(); ++itr) {
      image = (RQimage *)(&(*itr));
      dimMap.insert(map<string, RQnode *>::value_type(image->getIndexStr(), image));
      indexMap.insert(map<int, RQnode *>::value_type(indexMap.size()+1, image));
    }
    sortNodes(sort);
   } else {
    char str[32];
    for(itr=l->begin(); itr != l->end(); ++itr) {
      sprintf(str, "%d 0 0", (int) dimMap.size()+1);
      image = (RQimage *)(&(*itr));
      dimMap.insert(map<string, RQnode *>::value_type(string(str), image));
      indexMap.insert(map<int, RQnode *>::value_type(indexMap.size()+1, image));
    }
   }
}

//given the path of a group, make node for all images.
//this is done when making a new group.
void
RQgroup::makeChildren(const char *path)
{
    const int BUFLEN = 1024;
    char str[BUFLEN];
    struct stat fstat;
    const char *found;
    struct dirent **namelist;
    int n;

    if(stat(path, &fstat) != 0) {
        sprintf(str, "%s: \"%.512s\"", strerror(errno), path);
        ib_errmsg(str);
        return;
    }

    FILE *fp;
    char  buf[MAXSTR];
    sprintf(buf,"%s/%s",path,"/imgList");
    if(stat(buf, &fstat) == 0 && (fp = fopen(buf, "r"))) {
      while (fgets(buf,sizeof(buf),fp)) {
        if(strlen(buf) > 1 && buf[0] != '#') {
           if(buf[0] != '/') {
              sprintf(str, "%s/",path);
	      strncat(str,buf,strlen(buf)-1);
           } else {
	      strncpy(str,buf,strlen(buf)-1);
	   }

           found = strstr(str, ".fdf");
           if(stat(str, &fstat) == 0 && S_ISREG(fstat.st_mode) && found) {
	      RQimage image = RQimage(this, string(str) + " 0");
              if(image.getAttribute("rank") == "2" || image.getAttribute("rank") == "3") addChild(image);
	   }
	}
      }
      doSort = false; // set this flag so images won't be sorted
      return;
    }

    doSort = true;

    found = strstr(path, ".fdf");
    if(S_ISREG(fstat.st_mode) && found && strlen(found) == 4) {
        string newpath = path;
        newpath = newpath.substr(0,newpath.find_last_of("/"));
	makeChildren(newpath.c_str());
    } else if (S_ISDIR(fstat.st_mode)) {
                                                                                
      // Open the directory
      n = scandir(path, &namelist, 0, alphasort);
      if(n < 0) {
        sprintf(str, "%s: \"%.512s\"", strerror(errno), path);
        ib_errmsg(str);
        return;
      }
      // Loop through the file list looking for fdf files.
      for(int i=0; i<n; i++) {
	if (*(namelist[i]->d_name) != '.') {
          sprintf(str, "%s/%s",path,namelist[i]->d_name);
          found = strstr(str, ".fdf");
          if(stat(str, &fstat) == 0 &&
          S_ISREG(fstat.st_mode) && found && strlen(found) == 4) {
            RQimage image = RQimage(this, string(str) + " 0");
            if(image.getAttribute("rank") == "2" || image.getAttribute("rank") == "3") addChild(image);
	  }
        }
        free(namelist[i]);
      }
      free(namelist);                                                          
    }
}

// called in RQgroup constructor to get info when missing from fdf header.
int
RQgroup::getDimFromProcpar(string path, int *slices, int *array_dim, int *echoes)
{
// old header does not have correct array_dim.
// read from procpar file.

    int n=5;
    string pars[5];
    double vals[5];
    pars[0] = "pss";
    pars[1] = "ne";
    pars[2] = "nv";
    pars[3] = "arraydim";
    pars[4] = "ni";
    for(int i=0; i<n; i++) {
       vals[i] = 1;
    }
    getRealFromProcpar(path, pars, vals, n);

    *slices = (int) vals[0];
    *echoes = (int) vals[1];
    if(vals[4] > 1 && vals[2] != 0 && vals[2] <= vals[3])
       *array_dim = (int) (vals[3]/vals[2]);
    else
       *array_dim = (int) vals[3];

    return 0;
}

void
RQgroup::getRealFromProcpar(string path, string *pars, double *vals, int n)
{
    char *strptr;
    char  buf[1024];

    size_t i = path.find_last_of("/procpar",1);
    if(i == string::npos || i < (path.length()-9)) {
      path += "/procpar";
    }

    FILE* fp;
    if(!(fp = fopen(path.c_str(),"r"))) return;

    while(fgets(buf,sizeof(buf),fp)) {
        strptr = strtok(buf, " ");

        for(int i=0; i<n; i++) {
         if(strcmp(strptr, pars[i].c_str()) == 0) {
            fgets(buf,sizeof(buf),fp);
            strptr = strtok(buf, " ");
	    if(strcmp(pars[i].c_str(),"pss") != 0) strptr = strtok(NULL, " ");
            vals[i] = atoi(strptr);
         }
        }
    }
    fclose(fp);
}

void
RQgroup::setSelection(string str)
{
    string istr="NA";
    string sstr="NA";
    string astr="NA";
    string estr="NA";
    string fstr="NA";

    size_t p1, p2;
    p2 = 0;
    if((p1 = str.find(" ", 1)+1) != string::npos &&
           (p2 = str.find(" ", p1+1)) != string::npos)
            istr = str.substr(p1, p2-p1);
    p1 = p2+1;
    if(p1 != string::npos &&
          (p2 = str.find(" ", p1+1)) != string::npos)
            sstr = str.substr(p1, p2-p1);
    p1 = p2+1;
    if(p1 != string::npos &&
          (p2 = str.find(" ", p1+1)) != string::npos)
            astr = str.substr(p1, p2-p1);
    p1 = p2+1;
    if(p1 != string::npos &&
          (p2 = str.find(" ", p1+1)) != string::npos)
            estr = str.substr(p1, p2-p1);

    if(p2 != string::npos) fstr = str.substr(p2+1);

    if(istr != "NA") setImageSelection(istr);
    if(istr != "NA") setSliceSelection(sstr);
    if(istr != "NA") setArraySelection(astr);
    if(istr != "NA") setEchoSelection(estr);
    if(istr != "NA") setFrameSelection(fstr);
}

void
RQgroup::setImageSelection(string str)
{
    setAttribute("images", str);
}

void
RQgroup::setSliceSelection(string str)
{
    if(str != "NA" && str != "") { 
       setAttribute("slices", str);
       setAttribute("images", "");
    } else if(str == "NA") setAttribute("slices", "all");
}

void
RQgroup::setEchoSelection(string str)
{
    if(str != "NA" && str != "") { 
       setAttribute("echoes", str);
       setAttribute("images", "");
    } else if(str == "NA") setAttribute("echo", "all");
}

void
RQgroup::setArraySelection(string str)
{
    if(str != "NA" && str != "") { 
       setAttribute("array", str);
       setAttribute("images", "");
    } else if(str == "NA") setAttribute("array", "all");
}

void
RQgroup::setFrameSelection(string str)
{
    if(str != "NA" && str != "")
       setAttribute("frames", str);
}

bool
RQgroup::hasKey(string k) 
{
    ChildList *l = getChildren();
    ChildList::iterator itr;
    for(itr=l->begin(); itr != l->end(); ++itr) {
	if(itr->getKey() == k) return true;
    } 
    return false;
}

bool
RQgroup::hasImage(string k)
{
    list<RQnode *>::iterator itr;
    for(itr=imageList.begin(); itr != imageList.end(); ++itr) {
        if(k == (*itr)->getKey()) return true;
    }
    return false;
}

void
RQgroup::deleteImage(string key)
{
    list<RQnode *>::iterator itr;
    for(itr=imageList.begin(); itr != imageList.end(); ++itr) {
        if((*itr)->getKey() == key) {
            imageList.erase(itr);
            break;
        }
    }
}

// str may be a selection or the return string of "parseSentence",
// which has 6 fields and the 2-5 are for images, slices, array, echoes.
// use imageSelection, sliceSelection etc. if str is "selected".
void
RQgroup::updateImageList(string str) 
{
    imageList.clear();

    if(indexMap.size() <= 0) return;
    
    map<int, RQnode *>::iterator iitr;

    if(strcasecmp(str.c_str(),"all") == 0) {
       if(doSort) {
         for (iitr = indexMap.begin(); iitr != indexMap.end(); ++iitr) 
         {
	   imageList.push_back(iitr->second);
	 }
       } else {
          ChildList *l = getChildren();
          list<RQnode>::iterator citr;
          for(citr = l->begin(); citr != l->end(); ++citr) { 
	   imageList.push_back(&(*citr));
          }
       }
    	return;
    } 

    string istr = "1";
    string sstr = "1";
    string astr = "1";
    string estr = "1";
    size_t p1, p2;
    if(str == "" || strcasecmp(str.c_str(),"selected") == 0) {
	istr = getAttribute("images");
	sstr = getAttribute("slices");
	astr = getAttribute("array");
	estr = getAttribute("echoes");
    } else if((p1 = str.find_first_of(" ")) == string::npos) {
	istr = str;
    } else { // string of 6 fields.
        if((p1 = str.find(" ", 1)+1) != string::npos &&
           (p2 = str.find(" ", p1+1)) != string::npos)
            istr = str.substr(p1, p2-p1);
        p1 = p2+1;
        if(p1 != string::npos &&
          (p2 = str.find(" ", p1+1)) != string::npos)
            sstr = str.substr(p1, p2-p1);
        p1 = p2+1;
        if(p1 != string::npos &&
          (p2 = str.find(" ", p1+1)) != string::npos)
            astr = str.substr(p1, p2-p1);
        p1 = p2+1;
        if(p1 != string::npos &&
          (p2 = str.find(" ", p1+1)) != string::npos)
            estr = str.substr(p1, p2-p1);
    }

    list<int> l;
    l = RQparser::get()->parseImages(istr, getGroupSize());
    list<int>::iterator itr;
    if(l.size() > 0) {
        for (itr = l.begin(); itr != l.end(); ++itr) { 
	    iitr = indexMap.find(*itr);
	    if(iitr != indexMap.end()) {
	       imageList.push_back(iitr->second);
	    } 
	}
    } else if(dimMap.size() == indexMap.size()) {
      list<int> s = RQparser::get()->parseSlices(sstr, getSlices());
      list<int> a = RQparser::get()->parseSlices(astr, getArray_dim());
      list<int> e = RQparser::get()->parseSlices(estr, getEchoes());
      list<int>::iterator sitr;
      list<int>::iterator aitr;
      list<int>::iterator eitr;

      char ind[MAXSTR];
      map<string, RQnode *>::iterator iitr;
      for (sitr = s.begin(); sitr != s.end(); ++sitr) 
      for (aitr = a.begin(); aitr != a.end(); ++aitr) 
      for (eitr = e.begin(); eitr != e.end(); ++eitr) { 
 	sprintf(ind,"%d %d %d", *sitr, *aitr, *eitr);
	iitr = dimMap.find(string(ind));
	if(iitr != dimMap.end()) {
	   imageList.push_back(iitr->second);
	}		
      }
    } else {
	imageList.push_back(indexMap.begin()->second);
    } 
}

int
RQgroup::getSelSize() 
{
    return imageList.size();
}

void 
RQgroup::sortImages(string sort)
{
    if(strcasecmp(sort.c_str(), "yes") == 0) {
         setAttribute("sort", "eas");
         sortNodes(string("eas"));
    } else if(strcasecmp(sort.c_str(), "no") == 0) {
         setAttribute("sort", "sea");
         sortNodes(string("sea"));
    } else { 
         setAttribute("sort", sort);
         sortNodes(sort);
    }
}

void
RQgroup::sortNodes(string str) 
{
// some old image files does not have correct dimension indexes.
// don't sort images in this case.

    indexMap.clear();
    map<string, RQnode *>::iterator itr;

    // don't sort
    if(!canSort(str)) {
        int pos = 1;
	for(itr = dimMap.begin(); itr != dimMap.end(); ++itr) {
	  indexMap.insert(map<int, RQnode *>::value_type(pos, itr->second));
          pos++;
	}
        return;
    }

    int slices = getSlices();
    int array_dim = getArray_dim();
    int echoes = getEchoes();

    char ind[MAXSTR];
    if(str == "eas" ) {
        int pos = 1;
        for(int i=1; i<=echoes; i++)
        for(int j=1; j<=array_dim; j++)
        for(int k=1; k<=slices; k++) {
	    sprintf(ind, "%d %d %d", k, j, i);
	    itr = dimMap.find(string(ind));
	    if(itr != dimMap.end()) {
		indexMap.insert(map<int, RQnode *>::value_type(pos, itr->second));
	        pos++;
	    }
        }
    } else if(str == "esa" ) {
        int pos = 1;
        for(int i=1; i<=echoes; i++)
        for(int k=1; k<=slices; k++)
        for(int j=1; j<=array_dim; j++) {
	    sprintf(ind, "%d %d %d", k, j, i);
	    itr = dimMap.find(string(ind));
	    if(itr != dimMap.end()) {
		indexMap.insert(map<int, RQnode *>::value_type(pos, itr->second));
	        pos++;
	    }
        }
    } else if(str == "sea" ) {
        int pos = 1;
        for(int k=1; k<=slices; k++)
        for(int i=1; i<=echoes; i++)
        for(int j=1; j<=array_dim; j++) {
	    sprintf(ind, "%d %d %d", k, j, i);
	    itr = dimMap.find(string(ind));
	    if(itr != dimMap.end()) {
		indexMap.insert(map<int, RQnode *>::value_type(pos, itr->second));
	        pos++;
	    }
        }
    } else if(str == "sae" ) {
        int pos = 1;
        for(int k=1; k<=slices; k++)
        for(int j=1; j<=array_dim; j++)
        for(int i=1; i<=echoes; i++) {
	    sprintf(ind, "%d %d %d", k, j, i);
	    itr = dimMap.find(string(ind));
	    if(itr != dimMap.end()) {
		indexMap.insert(map<int, RQnode *>::value_type(pos, itr->second));
	        pos++;
	    }
        }
    } else if(str == "ase" ) {
        int pos = 1;
        for(int j=1; j<=array_dim; j++)
        for(int k=1; k<=slices; k++)
        for(int i=1; i<=echoes; i++) {
	    sprintf(ind, "%d %d %d", k, j, i);
	    itr = dimMap.find(string(ind));
	    if(itr != dimMap.end()) {
		indexMap.insert(map<int, RQnode *>::value_type(pos, itr->second));
	        pos++;
	    }
        }
    } else if(str == "aes" ) {
        int pos = 1;
        for(int j=1; j<=array_dim; j++)
        for(int i=1; i<=echoes; i++)
        for(int k=1; k<=slices; k++) {
	    sprintf(ind, "%d %d %d", k, j, i);
	    itr = dimMap.find(string(ind));
	    if(itr != dimMap.end()) {
		indexMap.insert(map<int, RQnode *>::value_type(pos, itr->second));
	        pos++;
	    }
        }
    }
    if(indexMap.size() != dimMap.size()) {
         setAttribute("sort", "no");
         sortNodes(string("no"));
    }
}

/*
// str may be a selection or the return string of "parseSentence",
// which has 6 fields and the last is for frames.
// use frameSelection if str is "selected".
int
RQgroup::updateFrames(string str)
{
    string frameSelection = getAttribute("frames");

// make image selection and frame layout before calling this method.
    int rows = (int)getReal("aipWindowSplit", 1, 1);
    int cols = (int)getReal("aipWindowSplit", 2, 1);
    int nf = rows*cols;
    if(nf <= 0) {
	GframeManager::get()->splitWindow(1,1);
	rows = 1;
	cols = 1;
	nf = 1;
    } 
    string frames;
    int p; 
    if(str == "" || strcasecmp(str.c_str(), "selected") == 0) frames = frameSelection;
    else if((p = str.find_last_of(" ")) == string::npos) frames = str; 
    else frames = str.substr(p+1); 
    list<int> l = RQparser::get()->parseFrames(frames, rows, cols, getGroupSize());

    if(l.size() <= 0) {
	for(int i=0; i<nf; i++) l.push_back(i);
    }
    
    list<int>::iterator fitr;

    list<RQnode *>::iterator iitr;
    fitr = l.begin(); 
    int batch = 1;
    int b;
    int f;
    for (iitr = imageList.begin(); iitr != imageList.end(); ++iitr) {
	if(fitr == l.end()) {
	   batch++;
	   fitr = l.begin(); 
	}
	if((*fitr) > nf) {
	   f = (*fitr)%nf;
           if(f == 0) {
		f = nf;
	        b = batch + (*fitr)/nf - 1;
	   } else {
	        b = batch + (*fitr)/nf;
	   }
	   (*iitr)->setAttribute("frame", f);
	   (*iitr)->setAttribute("batch", b);
	} else {
	   (*iitr)->setAttribute("frame", *fitr);
	   b = batch;
	   (*iitr)->setAttribute("batch", b);
	}
	fitr++;
    }
    return b;
}
*/

RQnode *
RQgroup::getImageNode(string key)
{
    return getChildByKey(key);
}

/*
int
RQgroup::getMaxFrame()
{
     int i;
     string str;
     string frameSelection = getAttribute("frames");

     if(frameSelection.length() <= 0 || 
	strcasecmp(frameSelection.c_str(), "NA") == 0) {
	return 1; 
     } else if(strcasecmp(frameSelection.c_str(), "all") == 0 ||
	strcasecmp(frameSelection.c_str(), "1-") == 0 || 
	strstr(frameSelection.c_str(), "r") != NULL || 
	strstr(frameSelection.c_str(), "c") != NULL || 
	strstr(frameSelection.c_str(), "R") != NULL || 
	strstr(frameSelection.c_str(), "C") != NULL) { 
	return imageList.size();
     } else {
	i = frameSelection.find("-",1);
	i = atoi(frameSelection.substr(0,i).c_str());
	list<int> l = RQparser::get()->parseFrames(frameSelection,
		imageList.size(), 1, imageList.size());
	list<int>::iterator itr;
	int mf = 0;
	for(itr = l.begin(); itr != l.end(); ++itr) {
	    if(*itr > mf) mf = *itr;
	}	
	return mf;
     }
}
*/

int
RQgroup::getScanSize() 
{
   return atoi(getAttribute("ns").c_str())
		*atoi(getAttribute("na").c_str())
		*atoi(getAttribute("ne").c_str());
}

int
RQgroup::getGroupSize() 
{
   return getChildrenCount();
}

int
RQgroup::getGnum()
{
    string gid = getAttribute("group");
    size_t pos = gid.find_first_of("(");
    if(pos != string::npos && pos > 1)
         return atoi(gid.substr(1,gid.length()-pos).c_str());
    else return -1;
}

// str is the key of an image
void RQgroup::setDirAndName(string str)
{
      size_t p1 = str.find_first_of(" ");
      size_t p2 = str.find(".fdf");
      string gpath;
      // get group path
      if(p1 != string::npos && p2 != string::npos) { // image key
        gpath = str.substr(0,p1);
      } else if(p1 != string::npos) { // group key
        gpath = str;
        gpath.replace(gpath.find_first_of(" "), 1, "/");
        p1 = gpath.find_first_of(" ");
        if(p1 != string::npos) gpath = str.substr(0,p1);
      } else if(p2 != string::npos) { // image path
        p1 = str.find_last_of("/");
        gpath = str.substr(0,p1);
      } else { // group path (.img or .fid)
        gpath = str;
      }
      p1 = gpath.find_last_of("/");
      string path = gpath.substr(0,p1);
      string dir = path;
      string name = gpath.substr(dir.length()+1);

      // special case if name ends with maps
      if(name.find("maps") == (name.length()-4)) {
        p1 = dir.find_last_of("/");
        dir = dir.substr(0,p1);
        name = gpath.substr(dir.length()+1);
      }
      // special case if dir ends with maps
      if(dir.find("maps") == (dir.length()-4)) {
        p1 = dir.find_last_of("/");
        dir = dir.substr(0,p1);
        name = gpath.substr(dir.length()+1);
      }
      // special case if dir ends with /data 
      if(dir.find("/data") == (dir.length()-5)) {
        p1 = dir.find_last_of("/");
        dir = dir.substr(0,p1);
        name = gpath.substr(dir.length()+1);
      }
      
      setAttribute("dir",dir);
      setAttribute("name",name);   
}

string
RQgroup::getStudyPath()
{
    string path = getAttribute("dir");
    return path;
}

void
RQgroup::updateIndexMap()
{
    indexMap.clear();
    ChildList *l = getChildren();
    list<RQnode>::iterator itr;
    for(itr = l->begin(); itr != l->end(); ++itr) { 
      indexMap.insert(map<int, RQnode *>::value_type(indexMap.size()+1, &(*itr)));
    }
}

int
RQgroup::resetChildren()
{
    getChildren()->clear();
    makeChildren(getPath().c_str());
    makeMaps();
    //makeShortNames();
/*
    if(dimMap.size() != getScanSize()) return 0;

    ChildList *l = getChildren();
    l->clear();
    indexMap.clear();

    int slices = getSlices();
    int array_dim = getArray_dim();
    int echoes = getEchoes();
    map<string, RQnode *>::iterator itr;
    string sort = getAttribute("sort");

    int pos = 1;
    char ind[MAXSTR];
    if(strcasecmp(sort.c_str(), "yes") == 0) { // eas
     for(int i=1; i<=echoes; i++)
     for(int j=1; j<=array_dim; j++)
     for(int k=1; k<=slices; k++) {
      sprintf(ind, "%d %d %d", k, j, i);
      itr = dimMap.find(string(ind));
      if(itr != dimMap.end()) {
	l->push_back(itr->second);
	indexMap.insert(map<int, RQnode *>::value_type(pos, itr->second));
      }
      pos++;
     }
    } else if(strcasecmp(sort.c_str(), "no") == 0) { // sea
      for(int k=1; k<=slices; k++)
      for(int i=1; i<=echoes; i++)
      for(int j=1; j<=array_dim; j++) {
      sprintf(ind, "%d %d %d", k, j, i);
      itr = dimMap.find(string(ind));
      if(itr != dimMap.end()) {
	l->push_back(itr->second);
	indexMap.insert(map<int, RQnode *>::value_type(pos, itr->second));
      }
      pos++;
     }
    }
*/
    return getChildrenCount();
}

void
RQgroup::makeShortNames()
{
    list<string> paths;
    DataMap::iterator pd;
    string::size_type idx;

    ChildList *l = getChildren();
    list<RQnode>::iterator itr;
    for(itr=l->begin(); itr != l->end(); ++itr) {
        string path(itr->getPath());
        paths.push_back(path);
    } 

    int len = paths.size();
    if (len == 0) {
        return;
    }       

    list<string>::iterator pname;
    /*for (pname = paths.begin(); pname != paths.end(); ++pname) {
        fprintf(stderr,"%s\n", pname->c_str());
    }*/

    // Strip suffixes
    for (pname = paths.begin(); pname != paths.end(); ++pname) {
        //idx = pname->rfind('.');
        if ((idx = pname->rfind(".fdf")) != string::npos
            || (idx = pname->rfind(".FDF")) != string::npos)
        {
            pname->erase(idx, 4);
        }
        //fprintf(stderr,"%s\n", pname->c_str());
    }

    // Strip identical leading components
    int nMatch = 0;                  // Strings match up to this point
    idx = 1; // Position in string to start checking from; posn of next "/"
    string fname = *paths.begin();
    while (idx < fname.length()) {
        idx = fname.find('/', idx);
        if (idx == string::npos) {
            break;
        }
        bool match = true;
        for (pname = paths.begin(), ++pname; pname != paths.end(); ++pname) {
            if (fname.compare(nMatch, idx - nMatch + 1,
                              *pname, nMatch, idx - nMatch + 1) == 0)
            {
                match = true;
            } else {
                match = false;
                break;
            }
        }
        if (match) {
            nMatch  = idx++;
        } else {
            break;
        }
    }
    if (nMatch > 0) {
        for (pname = paths.begin(); pname != paths.end(); ++pname) {
            pname->erase(0, nMatch + 1);
            //fprintf(stderr,"%s\n", pname->c_str());
        }
    }

    // Strip leading "image" from last component
    string junk = "image";
    int junklen = junk.length();
    for (pname = paths.begin(); pname != paths.end(); ++pname) {
        if ((idx = pname->rfind('/')) != string::npos) {
            ++idx;
        } else {
            idx = 0;
        }
        if (pname->substr(idx, junklen) == junk) {
            pname->erase(idx, junklen);
        }
    }

    // Shorten each component in various ways; turn "/"s into spaces
    junk = ".dat";
    junklen = junk.length();
    for (pname = paths.begin(); pname != paths.end(); ++pname) {
        idx = 0;                // Current position in string
        *pname += "/";          // Add trailing "/" for book keeping
        while (idx < pname->length()
               && (idx = pname->find('/', idx)) != string::npos)
        {
            // Strip trailing ".dat" from any component, if present
            if ((int) idx > junklen && pname->substr(idx - junklen, junklen) == junk) {
                pname->erase(idx - junklen, junklen);
                idx -= junklen;
            }

            // Strip leading "0"s from trailing integers in any component
            int i;
            int j;
            for (i = idx - 1; i >= 0 && isdigit(pname->at(i)); --i);
            ++i;
            for (j = i; j < (int) idx - 1 && pname->at(j) == '0'; ++j);
            if (j > i) {
                pname->erase(i, j - i);
                idx -= j - i;
            }

            // Replace "/" path delimiter with a space
            pname->replace(idx, 1, " ");
        }
        pname->resize(pname->length() - 1); // Remove the char we added
        //fprintf(stderr,"%s\n", pname->c_str());
    }

    // Save abbreviated names in the RQimage 
    ChildList *cl = getChildren();
    for(itr=cl->begin(), pname = paths.begin(); 
	itr != cl->end() && pname != paths.end(); 
	++itr, ++pname) {
        itr->setAttribute("shortName", *pname);
    }
}

void
RQgroup::selectByPar(const char *name, int value)
{
    int val;
    spDataInfo_t dataInfo;
    map<int, RQnode *>::iterator iitr;

    imageList.clear();
    for (iitr = indexMap.begin(); iitr != indexMap.end(); ++iitr)
    {
    	ReviewQueue::get()->loadKey(iitr->second->getKey());
        dataInfo = DataManager::get()->getDataInfoByKey(iitr->second->getKey());
        if(dataInfo == (spDataInfo_t)NULL) continue;

        val = value + 1;
        dataInfo->st->GetValue(name, val);
        if(val == value) imageList.push_back(iitr->second);
    }
}
