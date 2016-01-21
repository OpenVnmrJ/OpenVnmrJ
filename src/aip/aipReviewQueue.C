/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

using namespace std;

#include "aipRQimage.h"
#include "aipRQgroup.h"
#include "aipRQstudy.h"
#include "aipReviewQueue.h"
#include "aipRQparser.h"
#include "aipRQparser.h"
#include "aipDataManager.h"
#include "aipGframeManager.h"
#include "aipDataInfo.h"
#include "aipVnmrFuncs.h"
#include "aipCommands.h"
#include "aipWinRotation.h"
#include "aipWinMovie.h"
#include "aipVolData.h"
#include "aipOrthoSlices.h"
using namespace aip;

ReviewQueue *ReviewQueue::reviewQueue= NULL;

void ReviewQueue::printTime(string label) {
    struct timeval tv1;
    gettimeofday(&tv1, 0);
    fprintf(stderr,"%s %f\n", label.c_str(), tv1.tv_sec + (tv1.tv_usec)* 1e-6);
}

ReviewQueue::ReviewQueue() {
    studies = new list<RQstudy>;
    m_batches = 1;
    m_batch = 1;
    m_dragging = 0;
    m_remakeImageList = false;

    setReal("aipBatches", m_batches, true);
    setReal("aipBatch", m_batch, true);
}

ReviewQueue *ReviewQueue::get() {
    if (!reviewQueue) {
        reviewQueue = new ReviewQueue();
    }
    return reviewQueue;
}

void ReviewQueue::readNodes(const char *path) {
    studies->clear();

    FILE* fp;
    if (!(fp = fopen(path, "r")))
        return;

    char buf[1024];
    string str;
    string s;

    RQstudy *study = (RQstudy *)NULL;
    RQgroup *group = (RQgroup *)NULL;
    while (fgets(buf, sizeof(buf),fp)) {
        str = buf;
        if(str.find("<filenode",0) == str.find_first_not_of(" ", 0)) {
            s = str.substr(str.find("type=",0)+6, 6);
            if(s.find("study",0) == 0) {
                // study's parent is (RQnode *)NULL;
                RQstudy s = RQstudy(str);
                studies->push_back(s);
                study = &(studies->back());
            } else if(s.find("scan",0) == 0 && study != (RQstudy *)NULL) {
                RQgroup node = RQgroup(str);
                // abort if has rank 3 group
                if(node.getAttribute("rank") == "3") {
                    studies->clear();
		    fclose(fp);
                    return;
                }
                study->addChild(node);
                group = (RQgroup *)(study->getChild(study->getChildrenCount()));
            } else if(s.find("img",0) == 0 && group != (RQgroup *)NULL) {
                RQimage node = RQimage(str);
                group->addChild(node);
            }
        }
    }
    fclose(fp);

    //update nid since image nodes are not readed (not complete), they are created by group. 
    updateStudies();
    notifyVJ("readNodes");
}

int ReviewQueue::writeNodes(const char *path) {
    ofstream fout;
    try {
      fout.open(path);
      if(!fout.good()) return 0;
    } catch (const ios::failure & problem) { 
        return 0;
    }

    fout << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"<< endl << endl;
    fout << "<!DOCTYPE template>"<< endl << endl;
    fout << "<template type=\"imgstudy\">"<< endl;

    list<RQstudy>::iterator itr;
    for (itr = studies->begin(); itr != studies->end(); ++itr) {
        itr->writeNode(fout);
    }
    fout << "</template>"<< endl;

    fout.close();
    return 1;
}

void ReviewQueue::updateStudies() {

    ChildList *gl;
    ChildList::iterator gitr;
    ChildList *il;
    ChildList::iterator iitr;
    list<RQstudy>::iterator sitr;
    int ns, na, is, ie, ia;

    // remove empty groups
/*
    for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
        gl = sitr->getChildren();
        gitr=gl->begin();
        while (gitr != gl->end()) {
            if (gitr->getChildrenCount() <= 0) {
                gl->erase(gitr);
                gitr=gl->begin();
                continue;
            }
            gitr++;
        }
    }
*/

    // remove empty studies
    sitr = studies->begin();
    while (sitr != studies->end()) {
        if (sitr->getChildren()->size() <= 0) {
            studies->erase(sitr);
            sitr = studies->begin();
            continue;
        }
        sitr++;
    }

    int ngroups = 0;
    for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
        gl = sitr->getChildren();
        for (gitr = gl->begin(); gitr != gl->end(); ++gitr) {
            if(gitr->getAttribute("display") == "yes") ngroups ++;
	}
    }

    nodeMap.clear();
    // update nid and group id
    char str[MAXSTR];
    int s = 1;
    int g = 1;
    int i = 1;
    int gn = 1;
    int t = 0;
    for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
        sprintf(str, "%d 0 0", s);
        sitr->setAttribute("nid", string(str));
        sitr->setParent((RQnode *)NULL);
        nodeMap.insert(map<string, RQnode *>::value_type(sitr->getKey(),
                &(*sitr)));

        g = 1;
        gl = sitr->getChildren();
        for (gitr = gl->begin(); gitr != gl->end(); ++gitr) {
            ((RQgroup *)(&(*gitr)))->makeMaps();
            sprintf(str, "%d %d 0", s, g);
            gitr->setAttribute("nid", string(str));
            gitr->setParent((RQnode *)(&(*sitr)));
            sprintf(str, "G%d(%d)", gn, gitr->getChildrenCount());
            gitr->setAttribute("group", string(str));
            nodeMap.insert(map<string, RQnode *>::value_type(gitr->getKey(),
                    &(*gitr)));

            il = gitr->getChildren();
            for (iitr = il->begin(); iitr != il->end(); ++iitr) {
                sprintf(str, "%d %d %d", s, g, i);
                iitr->setAttribute("nid", string(str));
                iitr->setParent((RQnode *)(&(*gitr)));
                ns = atoi(gitr->getAttribute("ns").c_str());
                na = atoi(gitr->getAttribute("na").c_str());
                is = atoi(iitr->getAttribute("ns").c_str());
                ie = atoi(iitr->getAttribute("ne").c_str());
                ia = atoi(iitr->getAttribute("na").c_str());
                i = ns*na*(ie-1)+ns*(ia-1)+is;
                if(ngroups > 1) sprintf(str, "G%d(%d)", gn, i);
		else sprintf(str, "%d",i);
                iitr->setAttribute("gindex", string(str));
                sprintf(str, "%d", t+i);
                iitr->setAttribute("lindex", string(str));
                nodeMap.insert(map<string, RQnode *>::value_type(
                        iitr->getKey(), &(*iitr)));
            }
            if(!VolData::get()->showingObliquePlanesPanel())
                t += ((RQgroup *)(&(*gitr)))->getScanSize();
                //t += gitr->getChildrenCount();
            g++;
            gn++;
        }
        s++;
    }
}

int ReviewQueue::getGnum(string gid) {
    // Example: gid can be 1, g1, G1 or G1(15). gn is 1

    int gn = atoi(gid.c_str());
    if (gn > 0)
        return gn;

    gn = -1;
    unsigned int pos = gid.find_first_of("(");
    if (pos != string::npos && pos > 1)
        gn = atoi(gid.substr(1,gid.length()-pos).c_str());
    else
        gn = atoi(gid.substr(1).c_str());

    return gn;
}

RQgroup *ReviewQueue::getRQgroup(string aname, string avalue) {
    RQgroup *group;
    ChildList::iterator gitr;
    ChildList *gl;

    list<RQstudy>::iterator sitr;
    for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
        gl = sitr->getChildren();
        for (gitr=gl->begin(); gitr != gl->end(); ++gitr) {
            group = (RQgroup *)(&(*gitr));
            if (aname == "imageKey"&& group->hasKey(avalue)) {
                return group;
            } else if (aname == "key"&& group->getKey() == avalue) {
                return group;
            } else if (aname == "path"&& group->getPath() == avalue) {
                return group;
            } else if (group->getAttribute(aname) == avalue) {
                return group;
            }
        }
    }
    return (RQgroup *)NULL;
}

RQgroup *ReviewQueue::getRQgroup(int gn) {
    RQgroup *group;
    ChildList *l;
    ChildList::iterator gitr;

    list<RQstudy>::iterator sitr;
    for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
        l = sitr->getChildren();
        for (gitr = l->begin(); gitr != l->end(); ++gitr) {
            group = (RQgroup *)(&(*gitr));
            if (group->getGnum() == gn) {
                return group;
            }
        }
    }
    return (RQgroup *)NULL;
}

RQnode *ReviewQueue::getRQnode(string key) {
    map<string, RQnode *>::iterator itr = nodeMap.find(key);

    if (itr != nodeMap.end())
        return itr->second;
    else
        return (RQnode *)NULL;
}

RQnode *ReviewQueue::getRQnode(int s, int g, int i) {
    // Example: 1 0 0 is study 1
    //          1 1 0 is group 1 in study 1
    //          1 1 1 is image 1 in group 1 in study 1

    char str[MAXSTR];
    sprintf(str, "%d %d %d", s, g, i);
    return getRQnode(getKey(string(str)));
}

RQnode *ReviewQueue::getRQnode(string aname, string avalue) {
    ChildList *gl;
    ChildList::iterator gitr;
    ChildList *il;
    ChildList::iterator iitr;

    list<RQstudy>::iterator sitr;
    for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
        if (aname == "key"&& sitr->getKey() == avalue)
            return &(*sitr);
        else if (sitr->getAttribute(aname) == avalue)
            return &(*sitr);

        gl = sitr->getChildren();
        for (gitr = gl->begin(); gitr != gl->end(); ++gitr) {
            if (aname == "key"&& gitr->getKey() == avalue)
                return &(*gitr);
            else if (gitr->getAttribute(aname) == avalue)
                return &*(gitr);

            il = gitr->getChildren();
            for (iitr = il->begin(); iitr != il->end(); ++iitr) {
                if (aname == "key"&& iitr->getKey() == avalue)
                    return &(*iitr);
                else if (iitr->getAttribute(aname) == avalue)
                    return &*(iitr);
            }
        }
    }
    return (RQnode *)NULL;
}

void ReviewQueue::notifyVJ(const char *action) {
    char xmlpath[MAXSTR];
    int fd;
    sprintf(xmlpath,"/tmp/RQtreeXXXXXX");
    fd = mkstemp(xmlpath);
    if(fd < 0) {
	Werrprintf("Cannot open %s",xmlpath);
        return;
    }
    if(!writeNodes(xmlpath))
    {
       close(fd);
       return;
    }
    close(fd);

    int vp = (int)getReal("jviewport", 1);

    char str[MAXSTR];
    sprintf(str, "RQ notify %d %s %s", vp, xmlpath, action);
    writelineToVnmrJ("vnmrjcmd", str);

    // now write the same xml file in persistence
    sprintf(xmlpath,"%s/persistence/RQtree%d.xml",userdir,vp);
    writeNodes(xmlpath);
}

int ReviewQueue::loadData(string path, string nid, int firstFrame, bool show) {
    if (VsInfo::getVsMode() != 0) {
        setString("aipVsFunction", "", false);
    }

    struct stat fstat;

    char str[MAXSTR];
    if (path.find("/", 0) != 0) {
        getcwd(str, MAXSTR);
        path = string(str) +"/"+ path;
    }

    if (stat(path.c_str(), &fstat) != 0) {
        sprintf(str, "RQ: file %s does not exist!\n", path.c_str());
        ib_errmsg(str);
        return 0;
    } else if (S_ISREG(fstat.st_mode)) {
        // Read in a single file
        int b = loadFile(path, nid, firstFrame, show);
        if (b == 0) {
            sprintf(str, "RQ: no image in %s\n", path.c_str());
            ib_errmsg(str);
        }
        return b;
    } else if (S_ISDIR(fstat.st_mode)) {
        int b = loadDir(path, nid, firstFrame, show);
        if (b == 0) {
            sprintf(str, "RQ: no images in %s\n", path.c_str());
            ib_errmsg(str);
        }
        return b;
    } else {
        sprintf(str, "RQ: cannot open file %s\n", path.c_str());
        ib_errmsg(str);
        return 0;
    }
}

// if firstFrame is not a valid number, get first selected frame, 
// or get first available frame, or get first frame
// also make sure there is at least 1 frame.
int ReviewQueue::getFrameToStart(int firstFrame) {

    GframeManager *gfm = GframeManager::get();
    int nframes = gfm->getNumberOfFrames();
    int autoLayout = getLayoutMode();
    if(nframes <1 && autoLayout) {
	nframes=1;
        gfm->splitWindow(1, 1);
    } else if(nframes <1) {
	int rows = (int)getReal("aipWindowSplit", 1, 1);
	int cols = (int)getReal("aipWindowSplit", 2, 1);
	nframes=rows*cols;
	gfm->splitWindow(rows,cols);
    }
    if(firstFrame > nframes && autoLayout) {
       double aspect = getAspectRatio();
       GframeManager::get()->splitWindow(firstFrame, aspect);
       nframes=firstFrame;
    } 

    if (firstFrame <= 0 || firstFrame > nframes) {
      spGframe_t gf;
      GframeList::iterator gfi;
      int i=0;
      for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf
            =gfm->getNextFrame(gfi)) {
	if(gf->selected) return i+1;
	i++;
      }
      firstFrame = gfm->getFirstAvailableFrame()+1;
      if(firstFrame <= 0 || firstFrame > nframes) firstFrame = 1;
      return firstFrame;

    } else return firstFrame;
}

bool ReviewQueue::hasKey(string key) {
    map<int, string>::iterator iitr;
    iitr = m_imageList.begin() ;
    for(iitr = m_imageList.begin(); iitr != m_imageList.end(); ++iitr) {
	if(iitr->second == key) return true;
    }
    return false;
}

int ReviewQueue::loadFile(string path, string nid, int firstFrame, bool show) {
    firstFrame = getFrameToStart(firstFrame);

    string key = path;
    if(key.find(" ") == string::npos) {
      key += " 0";
      key.replace(key.find_last_of("/"), 1, " ");
    }

    RQimage image = RQimage(key);

//    DataManager::get()->deleteDataByKey(key); // reload data when display

    if (loadKey(key) == "") return 0;

    spGframe_t gf = GframeManager::get()->getFrameByNumber(firstFrame);
    if(image.getAttribute("rank") == "2" && gf != nullFrame && gf->getFirstView() != nullView) {
	// load as overlay image
	gf->loadOverlayImg((char *)key.c_str(),"default.color");
	
        if(!hasKey(key))
        m_imageList.insert(map<int, string>::value_type(m_imageList.size(),key));
	return 1;
    }

    addImage(image, show);

    // check whether 2D data
    if (image.getAttribute("rank") == "2") {

        updateStudies();
        notifyVJ("loadFile");

        m_imageList.insert(map<int, string>::value_type(m_imageList.size(),key));

        if (show) {
          // display();
    	   DataMap *dmap = DataManager::get()->getDataMap();
    	   DataMap::iterator pd;
           GframeManager *gfm = GframeManager::get();
           gfm->setFrameToLoad(firstFrame-1);
           pd = dmap->find(key);
           if (pd != dmap->end()) {
             gfm->loadData(pd->second);
           }
        }
    }
    // 3D data will be extracted separately

    return 1;
}

void ReviewQueue::addImage(string key, bool display) {
    RQimage image = RQimage(key);
    if (image.getAttribute("rank") != "2")
        return;
    addImage(image, display);
}

void ReviewQueue::addImage(RQimage& image, bool display) {
    string nid = "";
    char str[MAXSTR];

    RQgroup *group = getRQgroup("path", image.getGroupPath());
    if (group != (RQgroup *)NULL) {
        if(!group->hasKey(image.getKey())) {
          //removeOldnode(&image, &nid);
          group->addChild(image);
          sprintf(str, "1-%d:1", group->getGroupSize());
          group->setAttribute("images",string(str));
 	}
        if (display) {
            group->setAttribute("display", "yes");
        } else {
            group->setAttribute("display", "no");
 	}
    } else {
        RQgroup g = RQgroup(image.getPath());
        g.addChild(image);
        if (display) {
            g.setAttribute("display", "yes");
        } else {
            g.setAttribute("display", "no");
 	}
        list<RQgroup> groups;
        groups.push_back(g);
        addGroups(groups, "");
    }
}

void ReviewQueue::addImage(RQimage& image, string nid) {
    removeOldnode(&image, &nid);

    RQnode *node = getNode(nid);
    if (node == (RQnode *)NULL) {
        appendNodes(image);
    } else {
        insertNode(image, node);
    }
}

void ReviewQueue::loadFid(string path) {
    list<RQgroup> groups;
/*
    string key = path + " 0";
    key.replace(key.find_last_of("/"), 1, " ");
    RQgroup group = RQgroup(key, 1, false);
*/
    RQgroup group = RQgroup(path, 1, false);
    groups.push_back(group);

    addGroups(groups, "");

    updateStudies();
    notifyVJ("loadDir");
}

int ReviewQueue::loadDir(string path, string nid, int firstFrame, bool show) {
// last argument true to set "display" yes.
    //firstFrame = getFrameToStart(firstFrame, true);
    if(firstFrame < 0) {
            unselectDisplay();
	    firstFrame = 1;
    }

    list<RQgroup> groups;

    // path could be a study path like: /..../s_xxxx
    // if is a study path and studyPath+"/study.xml" exists,
    // we get data paths (i.e., .img paths) from study.xml (so images are order beased on the queue).
    // otherwise recursively load images from given path.
    struct stat fstat;
    string studyPath = ""; 
    const char *found = strstr(path.c_str(), "/s_"); 
    if(found && strstr(++found,"/") == NULL) {
	studyPath = path + "/study.xml"; 
        if(stat(path.c_str(), &fstat) != 0)  studyPath = ""; 
    }

    FILE *fp;
    char  buf[MAXSTR];
    if(studyPath != "" && (fp = fopen(studyPath.c_str(), "r"))) { 
      string scanPath = ""; 
      string scanName = ""; 
      show=false;
      firstFrame = 1;
      while (fgets(buf,sizeof(buf),fp)) {
        if(strlen(buf) > 1 && (found=strstr(buf, "status=")) != NULL) {
	    found += 8;
	    if(strstr(found,"Completed") == found) {
              if(strlen(buf) > 1 && (found=strstr(buf, "data=")) != NULL) {
	        found += 6; // find start of scan name, i.g. data="data/epi_n002" or data="epi_n002"
		// load .img
                scanPath = string(found);
                scanName = scanPath.substr(0,scanPath.find_first_of("\""))+".img";
                scanPath = path + "/"+scanName;
                if(stat(scanPath.c_str(), &fstat) == 0) { // .img exists
                  makeGroups(scanPath.c_str(), groups, firstFrame+groups.size(), show);
		  continue;
		}
		// load .maps
                scanPath = string(found);
                scanName = scanPath.substr(0,scanPath.find_first_of("\""))+".maps";
                scanPath = path + "/"+scanName;
                if(stat(scanPath.c_str(), &fstat) == 0) { // .maps exists
                  makeGroups(scanPath.c_str(), groups, firstFrame+groups.size(), show);
		  continue;
		}
		// load .csi/maps
                scanPath = string(found);
 		scanName = scanPath.substr(0,scanPath.find_first_of("\""))+".csi/maps";
                scanPath = path + "/"+scanName;
                if(stat(scanPath.c_str(), &fstat) == 0) { // .maps exists
                  makeGroups(scanPath.c_str(), groups, firstFrame+groups.size(), show);
		  continue;
                }
                scanPath = string(found);
                scanName = scanPath.substr(0,scanPath.find_first_of("\""))+".fid";
                scanPath = path + "/"+scanName;
                if(stat(scanPath.c_str(), &fstat) == 0) { // .fid exists
		  RQgroup group = RQgroup(scanPath, 1, false);
    		  groups.push_back(group);
		}
	      }
	    }
        }
      }
      fclose(fp);
    } else {
       makeGroups(path.c_str(), groups, firstFrame, show);
    }

    if (groups.empty()) {
        fprintf(stderr, "No FDF files in directory: \"%.1024s\"", path.c_str());
        return 0;
    }

    int n = 0;
    list<RQgroup>::iterator itr;
    for (itr = groups.begin(); itr != groups.end(); ++itr) {
        n += itr->getChildrenCount();
    }

    addGroups(groups, nid);

    updateStudies();

    if (show && getReal("jviewport", 1) > 2.5) {
        selectImages(DATA_SELECTED_RQ);
        display();
    } else if (show) {
        selectImages(DATA_ALL);
        display();
    }
/*
        displaySel(DATA_SELECTED_RQ, getGlobalSort(), getLayoutMode());

        // update RQ panel
        ChildList *gl;
        ChildList::iterator gitr;
        list<RQstudy>::iterator sitr;
        int ff = 1;
        RQgroup *group;
        char str[64];
        for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
            gl = sitr->getChildren();
            for (gitr = gl->begin(); gitr != gl->end(); ++gitr) {
                group = ((RQgroup *)(&(*gitr)));
                if (group->getAttribute("display") == "yes") {
                    sprintf(str, "%d-", ff);
                    group->setAttribute("frames", string(str));
                    ff += group->getSelSize();
                }
            }
        }
    } else if (show) {
        displaySel(DATA_ALL, getGlobalSort(), getLayoutMode());
    }
*/

    notifyVJ("loadDir");

    return 1;
}

string ReviewQueue::getRQSelection() {
    list<RQstudy>::iterator itr;
    list<RQnode>::iterator gitr;
    list<RQnode> *gl;
    unsigned int ni = 0;
    int ng = 0;
    map<int, string> gmap;
    char sel[MAXSTR];
    for (itr = studies->begin(); itr != studies->end(); ++itr) {
        gl = itr->getChildren();
        for (gitr=gl->begin(); gitr != gl->end(); ++gitr) {
            ng++;
            ni += gitr->getChildrenCount();
            if (gitr->getAttribute("display") == "yes") {
                gmap.insert(map<int, string>::value_type(ng,
                        gitr->getAttribute("images")));
            }
        }
    }
    if (ni == m_imageList.size())
        strcpy(sel, "all");
    else if (gmap.size() == 1&& ng == 1)
        strcpy(sel, gmap.begin()->second.c_str());
    else {
        strcpy(sel, "");
        map<int, string>::iterator mitr;
        for (mitr = gmap.begin(); mitr != gmap.end(); ++mitr) {
	    char tmp[MAXSTR];
            sprintf(tmp, "g%d(%s)", mitr->first, mitr->second.c_str());
	    strcat(sel,tmp);
        }
    }

    return string(sel);
}

void ReviewQueue::addGroups(list<RQgroup>& groups, string nid) {
    // here nid is key.
    list<RQgroup>::iterator gitr;
    for (gitr = groups.begin(); gitr != groups.end(); ++gitr) {
	string gpath = gitr->getPath();
        if(nid=="") nid = gitr->getKey();
        removeOldnode(&(*gitr), &nid);
        RQnode *node = getNode(nid);
        if (node == (RQnode *)NULL) {
            // add to existing study if belongs to
            list<RQstudy>::iterator itr;
            bool b = false;
            for (itr = studies->begin(); itr != studies->end(); ++itr) {
                if (itr->getPath() == gitr->getStudyPath()) {
                    itr->addChild(*gitr);
                    b = true;
                    break;
                }
            }
            // make a new study (not belongs)
            if (!b) {
                appendNodes(*gitr);
	    }
        } else {
            insertNode(*gitr, node);
        }
    }
}

void ReviewQueue::appendNodes(RQstudy& study) {
    studies->push_back(study);
}

void ReviewQueue::appendNodes(RQgroup& group) {
    RQstudy study = RQstudy(group.getStudyPath());
    study.addChild(group);
    studies->push_back(study);
}

void ReviewQueue::appendNodes(RQimage& image) {
    RQstudy study = RQstudy(image.getStudyPath());
    RQgroup group = RQgroup(image.getPath());
    group.addChild(image);
    study.addChild(group);
    studies->push_back(study);
}

void ReviewQueue::removeOldnode(RQstudy *study, string *nid) {
    // here nid is key.
    // remove old node of the same key as study.
    // if removed node's key is nid, set nid to next node.

    string key = study->getKey();
    list<RQstudy>::iterator itr;
    for (itr = studies->begin(); itr != studies->end(); ++itr) {
        string skey = itr->getKey();
        if (skey == key) {
            if (skey == (*nid)) {
                if (&(*itr) != &(studies->back())) {
                    ++itr;
                    *nid = itr->getKey();
                    --itr;
                } else {
                    *nid = "";
                }
            }
            studies->erase(itr);
            return;
        }
    }
}

void ReviewQueue::removeOldnode(RQgroup *group, string *nid) {
    string key = group->getKey();
    list<RQstudy>::iterator itr;
    list<RQnode>::iterator gitr;
    list<RQnode> *gl;
    for (itr = studies->begin(); itr != studies->end(); ++itr) {
        gl = itr->getChildren();
        for (gitr=gl->begin(); gitr != gl->end(); ++gitr) {
            string gkey = gitr->getKey();
            if (gkey == key) {
                if (gkey == *nid) {
                    if (&(*gitr) != &(gl->back())) {
                        ++gitr;
                        *nid = gitr->getKey();
                        --gitr;
                    } else if (&(*itr) != &(studies->back())) {
                        ++itr;
                        *nid = itr->getKey();
                    } else {
                        *nid = "";
                    }
                }
                gl->erase(gitr);
                return;
            }
        }
    }
}

void ReviewQueue::removeOldnode(RQimage *image, string *nid) {
    string key = image->getKey();
    list<RQstudy>::iterator itr;
    ChildList::iterator gitr;
    ChildList::iterator iitr;
    ChildList *gl;
    ChildList *il;
    for (itr = studies->begin(); itr != studies->end(); ++itr) {
        gl = itr->getChildren();
        for (gitr = gl->begin(); gitr != gl->end(); ++gitr) {
            il = gitr->getChildren();
            for (iitr = il->begin(); iitr != il->end(); ++iitr) {
                string ikey = iitr->getKey();
                if (ikey == key) {
                    if (ikey == *nid) {
                        if (&(*iitr) != &(il->back())) {
                            ++iitr;
                            *nid = iitr->getKey();
                            --iitr;
                        } else if (&(*gitr) != &(studies->back())) {
                            ++gitr;
                            *nid = gitr->getKey();
                        } else if (&(*itr) != &(studies->back())) {
                            ++itr;
                            *nid = itr->getKey();
                        } else {
                            *nid = "";
                        }
                    }
                    il->erase(iitr);
                    return;
                }
            }
        }
    }
}

void ReviewQueue::moveNode(RQstudy *study, string nid) {
    if (study == (RQstudy *)NULL)
        return;
    RQstudy newStudy = *study;
    removeNode(study);
    insertNode(newStudy, getNode(nid));
}

void ReviewQueue::moveNode(RQgroup *group, string nid) {
    if (group == (RQgroup *)NULL)
        return;
    RQgroup newGroup = *group;
    removeNode(group);
    insertNode(newGroup, getNode(nid));
}

void ReviewQueue::moveNode(RQimage *image, string nid) {
    if (image == (RQimage *)NULL)
        return;
    RQimage newImage = *image;
    removeNode(image);
    insertNode(newImage, getNode(nid));
}

void ReviewQueue::removeNode(RQstudy *study) {
    // also need to remove from nodeMap;

    if (study == (RQstudy *)NULL)
        return;
     
    string key = study->getKey();
    map<string, RQnode *>::iterator mitr;

    ChildList *gl;
    ChildList::iterator gitr;
    ChildList *il;
    ChildList::iterator iitr;
    list<RQstudy>::iterator sitr;
    for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
        if (sitr->getKey() == key) {
            mitr = nodeMap.find(key);
            if (mitr != nodeMap.end())
                nodeMap.erase(mitr);
            gl = sitr->getChildren();
            for (gitr = gl->begin(); gitr != gl->end(); ++gitr) {
                mitr = nodeMap.find(gitr->getKey());
                if (mitr != nodeMap.end())
                    nodeMap.erase(mitr);
                il = gitr->getChildren();
                for (iitr = il->begin(); iitr != il->end(); ++iitr) {
                    mitr = nodeMap.find(iitr->getKey());
                    if (mitr != nodeMap.end())
                        nodeMap.erase(mitr);
                }
            }
            studies->erase(sitr);
            return;
        }
    }
}

void ReviewQueue::removeNode(RQgroup *group) {
    if (group == (RQgroup *)NULL)
        return;
   
    string key = group->getKey();
    map<string, RQnode *>::iterator mitr;

    ChildList *gl;
    ChildList::iterator gitr;
    ChildList *il;
    ChildList::iterator iitr;
    list<RQstudy>::iterator sitr;
    for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
        gl = sitr->getChildren();
        for (gitr = gl->begin(); gitr != gl->end(); ++gitr) {
            if (gitr->getKey() == key) {
                mitr = nodeMap.find(key);
                if (mitr != nodeMap.end())
                    nodeMap.erase(mitr);
                il = gitr->getChildren();
                for (iitr = il->begin(); iitr != il->end(); ++iitr) {
                    mitr = nodeMap.find(iitr->getKey());
                    if (mitr != nodeMap.end())
                        nodeMap.erase(mitr);
                }
                gl->erase(gitr);
                return;
            }
        }
    }
}

void ReviewQueue::removeNode(RQimage *image) {
    if (image == (RQimage *)NULL)
        return;
   
    string key = image->getKey();
    map<string, RQnode *>::iterator mitr;

    ChildList *gl;
    ChildList::iterator gitr;
    ChildList *il;
    ChildList::iterator iitr;
    list<RQstudy>::iterator sitr;
    for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
        gl = sitr->getChildren();
        for (gitr = gl->begin(); gitr != gl->end(); ++gitr) {
            il = gitr->getChildren();
            for (iitr = il->begin(); iitr != il->end(); ++iitr) {
                if (iitr->getKey() == key) {
                    mitr = nodeMap.find(key);
                    if (mitr != nodeMap.end())
                        nodeMap.erase(mitr);
                    il->erase(iitr);
                    return;
                }
            }
        }
    }
}

void ReviewQueue::copyNode(RQstudy *study, string nid) {
    if (study == (RQstudy *)NULL)
        return;
    RQstudy newStudy = RQstudy(study->getPath());
    ChildList::iterator gitr;
    ChildList::iterator iitr;
    ChildList *gl = study->getChildren();
    ChildList *il;
    RQgroup newGroup;
    for (gitr = gl->begin(); gitr != gl->end(); ++gitr) {
        il = gitr->getChildren();
        if (il->size() > 0)
            newGroup = RQgroup(il->begin()->getPath());
        else
            newGroup = RQgroup(gitr->getPath());
        for (iitr = il->begin(); iitr != il->end(); ++iitr) {
            RQimage node = RQimage(iitr->getPath());
            newGroup.addChild(node);
        }
        newStudy.addChild(newGroup);
    }
    insertNode(newStudy, getNode(nid));
}

void ReviewQueue::copyNode(RQgroup *group, string nid) {
    if (group == (RQgroup *)NULL)
        return;
    RQgroup newGroup;
    ChildList *il = group->getChildren();
    if (il->size() > 0)
        newGroup = RQgroup(il->begin()->getPath());
    else
        newGroup = RQgroup(group->getPath());

    ChildList::iterator iitr;
    for (iitr = il->begin(); iitr != il->end(); ++iitr) {
        RQimage node = RQimage(iitr->getPath());
        newGroup.addChild(node);
    }
    insertNode(newGroup, getNode(nid));
}

void ReviewQueue::copyNode(RQimage *image, string nid) {
    if (image == (RQimage *)NULL)
        return;
    RQgroup newGroup = RQgroup(image->getPath());
    RQimage newImage = RQimage(image->getPath());
    newGroup.addChild(newImage);
    insertNode(newGroup, getNode(nid));
}

void ReviewQueue::insertNode(RQstudy& study, RQnode *node) {
    if (&study == (RQstudy *)NULL)
        return;

    if (node == (RQnode *)NULL) {
        appendNodes(study);
        return;
    }

    RQstudy *target = getInsertStudy(node);
    if (target == (RQstudy *)NULL) {
        appendNodes(study);
        return;
    } else {
        list<RQstudy>::iterator sitr;
        for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
            if ((&(*sitr)) == target) {
                studies->insert(sitr, study);
                return;
            }
        }
    }
}

void ReviewQueue::insertNode(RQgroup& group, RQnode *node) {
    if (&group == (RQgroup *)NULL)
        return;
    if (node == (RQnode *)NULL) {
        appendNodes(group);
        return;
    }

    string type = node->getAttribute("type");
    if (type == "study") {
        list<RQstudy>::iterator sitr;
        for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
            if ((&(*sitr)) == node && sitr != studies->begin()) {
                --sitr;
                if (sitr != studies->end() &&sitr->getPath()
                        == group.getStudyPath()) {
                    sitr->addChild(group);
                    return;
                }
                ++sitr;
            }
        }
        RQstudy newStudy = RQstudy(group.getStudyPath());
        newStudy.addChild(group);
        insertNode(newStudy, node);
        return;
    } else if (type == "scan") {
        if (group.getStudyPath() == ((RQgroup *)node)->getStudyPath()) {
            list<RQnode> *gl = node->getParent()->getChildren();
            list<RQnode>::iterator gitr;
            for (gitr=gl->begin(); gitr != gl->end(); ++gitr) {
                if ((&(*gitr)) == node) {
                    node->getParent()->addChild(gitr->getAttribute("nid"), group);
                    return;
                }
            }
        }
        RQstudy newStudy = RQstudy(group.getStudyPath());
        newStudy.addChild(group);
        insertNode(newStudy, node);
        return;
    } else if (type == "img") {
        insertNode(group, getInsertGroup(node));
        return;
    }
}

void ReviewQueue::insertNode(RQimage& image, RQnode *node) {
    if (&image == (RQimage *)NULL)
        return;
    if (node == (RQnode *)NULL) {
        RQgroup newGroup = RQgroup(image.getGroupPath());
        newGroup.addChild(image);
        insertNode(newGroup, node);
        return;
    }

    string type = node->getAttribute("type");
    if (type == "study") {
        RQgroup newGroup = RQgroup(image.getGroupPath());
        newGroup.addChild(image);
        insertNode(newGroup, node);
        return;
    } else if (type == "scan") {
        list<RQnode> *gl = node->getParent()->getChildren();
        list<RQnode>::iterator gitr;
        for (gitr=gl->begin(); gitr != gl->end(); ++gitr) {
            if ((&(*gitr)) == node && gitr != gl->begin()) {
                --gitr;
                if (gitr != gl->end() &&gitr->getPath() == image.getGroupPath()) {
                    gitr->addChild(image);
                    return;
                }
                ++gitr;
            }
        }
        RQgroup newGroup = RQgroup(image.getGroupPath());
        newGroup.addChild(image);
        insertNode(newGroup, node);
        return;
    } else if (type == "img") {
        if (image.getGroupPath() == node->getParent()->getPath()) {
            list<RQnode> *il = node->getParent()->getChildren();
            list<RQnode>::iterator iitr;
            for (iitr=il->begin(); iitr != il->end(); ++iitr) {
                if ((&(*iitr)) == node) {
                    node->getParent()->addChild(iitr->getAttribute("nid"), image);
                    return;
                }
            }
        }
        RQgroup newGroup = RQgroup(image.getPath());
        newGroup.addChild(image);
        insertNode(newGroup, node);

        return;
    }
}

string ReviewQueue::getNextCopy(int type, string path) {
    int n = 0;
    char str[MAXSTR];

    ChildList *il;
    ChildList::iterator iitr;
    ChildList *gl;
    ChildList::iterator gitr;
    list<RQstudy>::iterator sitr;
    string key;

    switch (type) {
    case STUDY:
        for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
            if (sitr->getPath() == path)
                n++;
        }
        break;
    case SCAN:
        for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
            gl = sitr->getChildren();
            for (gitr = gl->begin(); gitr != gl->end(); ++gitr) {
                if (gitr->getPath() == path)
                    n++;
            }
        }
        break;
    case IMAGE:
        for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
            gl = sitr->getChildren();
            for (gitr = gl->begin(); gitr != gl->end(); ++gitr) {
                il = gitr->getChildren();
                for (iitr = il->begin(); iitr != il->end(); ++iitr) {
                    if (iitr->getPath() == path)
                        n++;
                }
            }
        }
        break;
    default:
        break;

    }

    sprintf(str, " %d", n);
    return string(str);
}

RQgroup *ReviewQueue::getInsertGroup(RQnode *node) {
    if (node == (RQnode *)NULL)
        return (RQgroup *)node;

    string type = node->getAttribute("type");

    if (type == "study") {
        return (RQgroup *)NULL;
    } else if (type == "scan") {
        return (RQgroup *)node;
    } else if (type == "img") {
        RQgroup *group = (RQgroup *)node->getParent();
        list<RQnode> *children = group->getChildren();
        list<RQnode>::iterator itr;
        RQgroup newGroup = RQgroup(node->getPath());
        newGroup.setParent(node->getParent());
        itr=children->begin();
        while ( (&(*itr)) != node && itr != children->end()) {
            newGroup.addChild(*itr);
            children->erase(itr);
            itr = children->begin();
        }
        if (newGroup.getChildrenCount() == 0)
            return group;
        RQstudy *study = (RQstudy *)(node->getParent()->getParent());
        study->addChild(group->getAttribute("nid"), newGroup);
        return group;
    }
    return (RQgroup *)NULL;
}

RQstudy *ReviewQueue::getInsertStudy(RQnode *node) {
    if (node == (RQnode *)NULL)
        return (RQstudy *)node;

    string type = node->getAttribute("type");

    if (type == "study") {
        return (RQstudy *)node;
    } else if (type == "scan") {
        RQstudy *study = (RQstudy *)(node->getParent());
        list<RQnode> *children = study->getChildren();
        list<RQnode>::iterator itr;
        RQstudy newStudy = RQstudy(study->getPath());
        itr=children->begin();
        while ( (&(*itr)) != node && itr != children->end()) {
            newStudy.addChild(*itr);
            children->erase(itr);
            itr = children->begin();
        }
        if (newStudy.getChildrenCount() == 0)
            return study;

        list<RQstudy>::iterator sitr;
        for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
            if (&(*sitr) == study) {
                studies->insert(sitr, newStudy);
                return study;
            }
        }
    } else if (type == "img") {
        return getInsertStudy(getInsertGroup(node));
    }
    return (RQstudy *)NULL;
}

void ReviewQueue::makeGroups(const char *path, std::list<RQgroup>& groups,
        int firstFrame, bool show) {
    const int BUFLEN = 1024;
    char str[BUFLEN],buf[BUFLEN];
    struct stat fstat;
    const char *found;
    string key;
    struct dirent **namelist;
    int n;
    FILE *fp;

    // use aipList if exist in given path.
    sprintf(buf,"%s/%s",path,"/aipList");
    if(stat(buf, &fstat) == 0 && (fp = fopen(buf, "r"))) {
          while (fgets(buf,sizeof(buf),fp)) {
            if(strlen(buf) > 1 && buf[0] != '#') {
              if(buf[0] != '/') {
                sprintf(str, "%s/",path);
                strncat(str,buf,strlen(buf)-1);
              } else {
                strncpy(str,buf,strlen(buf)-1);
              }
              if(stat(str, &fstat) == 0) { // aipList exists
                  makeGroups(str, groups, firstFrame+groups.size(), show);
	      }
	    }
	  }
	  fclose(fp);
	  return;
    } 

    if (stat(path, &fstat) != 0) {
        sprintf(str, "%s: \"%.1024s\"", strerror(errno), path);
        ib_errmsg(str);
        return;
    }

    if (S_ISDIR(fstat.st_mode)) {
        n = scandir(path, &namelist, 0, alphasort);
        if (n < 0) {
            sprintf(str, "%s: \"%.1024s\"", strerror(errno), path);
            ib_errmsg(str);
            return;
        }
        for(int i=0; i<n; i++) {
            if(interuption) {
		free(namelist[i]);
                while ((++i)<n) free(namelist[i]);
		free(namelist);
		return;
	    }
            if (*(namelist[i]->d_name) != '.') {
                sprintf(str, "%s/%s", path, namelist[i]->d_name);
                if ((found = strstr(str, ".fdf")) && strlen(found) == 4) {

                    key = string(str) + " 0";
                    key.replace(key.find_last_of("/"), 1, " ");
                    string gpath = path;
                    list<RQgroup>::iterator gitr;

                    bool b = false;
                    int size = 0;
                    for (gitr=groups.begin(); gitr != groups.end(); ++gitr) {
                        size += gitr->getChildrenCount();
                        if ((*gitr).getPath() == gpath) {
                            b = true;
                            break;
                        }
                    }
                    if (!b) {
                        key = loadKey(key);
			if (key != "") {
                            groups.push_back(RQgroup(key, size+firstFrame, show));
		        }
                    }
                } else makeGroups(str, groups, firstFrame, show);
            }
            free(namelist[i]);
        }
        free(namelist);
    }
}

int ReviewQueue::getNumGroups() {
    list<RQnode>::iterator gitr;
    list<RQstudy>::iterator sitr;
    ChildList *gl;
    int i = 0;
    for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
        gl = sitr->getChildren();
        for (gitr=gl->begin(); gitr != gl->end(); ++gitr) {
            i++;
        }
    }
    return i;
}

int ReviewQueue::getNumImages() {
    ChildList::iterator gitr;
    ChildList *gl;
    list<RQstudy>::iterator sitr;
    int i = 0;
    for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
        gl = sitr->getChildren();
        for (gitr=gl->begin(); gitr != gl->end(); ++gitr) {
            i += gitr->getChildrenCount();
        }
    }
    return i;
}

void ReviewQueue::unselectDisplay() {
    list<RQstudy>::iterator sitr;
    ChildList::iterator gitr;
    ChildList * gl;

    for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
        gl = sitr->getChildren();
        for (gitr=gl->begin(); gitr != gl->end(); ++gitr) {
            gitr->setAttribute("display", "no");
        }
    }
    m_imageList.clear();
}

void ReviewQueue::setDragging(bool b) {
    // m_dragging = 0, not dragging
    // m_dragging = 1, dragging, but imagelist is not up to date. 
    // m_dragging = 2, dragging and imagelist is up to date.

    if (!b) {
        m_dragging = 0;
    } else if (m_dragging == 0) {
        m_dragging = 1;
    }
}

set<string> ReviewQueue::makeCachedImageSet() {
    if (m_dragging == 1)
        m_dragging = 2;

    set<string> keys;
    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;
    spViewInfo_t view;
    GframeCache_t::iterator gfi;

    for (gf=gfm->getFirstCachedFrame(gfi); gf != nullFrame; gf
            =gfm->getNextCachedFrame(gfi)) {
        if ((view = gf->getFirstView()) == nullView)
            continue;

        keys.insert(view->imgInfo->getDataInfo()->getKey());
    }
    return keys;
}

list<string> ReviewQueue::getKeylist(int mode, string groupPath) {
    list<string> keys;
    string key;
    list<RQstudy>::iterator sitr;
    list<RQnode>::iterator gitr;
    list<RQnode *>::iterator iitr;
    RQgroup *group;
    ChildList *sl;
    list<RQnode *> *gl;
    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;
    spViewInfo_t view;
    GframeList::iterator gfi;
    list<RQnode *>::iterator itr;
    map<int, string>::iterator mitr;
    switch (mode) {
       case DATA_ALL:
       case DATA_SELECTED_RQ:

         selectImages(mode);
         mitr = m_imageList.begin() ;
         for(mitr = m_imageList.begin(); mitr != m_imageList.end(); ++mitr) {
             keys.push_back(mitr->second.c_str());
	 } 
         break;

       case DATA_SELECTED_FRAMES:
         for (gf=gfm->getFirstSelectedFrame(gfi); gf != nullFrame; gf
                =gfm->getNextSelectedFrame(gfi)) {
            if (gfm->isFrameDisplayed(gf) && (view = gf->getFirstView())
                    != nullView)
	       keys.push_back(view->imgInfo->getDataInfo()->getKey());
         }
         break;

       case DATA_DISPLAYED:
         for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf
                =gfm->getNextFrame(gfi)) {
            if (gfm->isFrameDisplayed(gf) && (view = gf->getFirstView())
                    != nullView)
	       keys.push_back(view->imgInfo->getDataInfo()->getKey());
         }
         break;

       case DATA_GROUP:
         if(groupPath.length() < 1) {
	   groupPath = getActiveKey();
	   groupPath = groupPath.substr(0,groupPath.find_first_of(" "));
         }
         for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
           sl = sitr->getChildren();
           for (gitr=sl->begin(); gitr != sl->end(); ++gitr) {
              group = (RQgroup *)(&(*gitr));
              if (group->getPath() == groupPath) {
                group->updateImageList("all");
                gl = group->getImageList();
                for (iitr=gl->begin(); iitr != gl->end(); ++iitr) {
                 keys.push_back((*iitr)->getKey());
                }
	      }
           }
         }
         break;
         
       case DATA_SELECTED_IMAGES:
         makeSelections(getGlobalSort(), getLayoutMode(), getSelection());
         if (m_selectedImages.size() > 0) {
            for (itr = m_selectedImages.begin(); itr != m_selectedImages.end(); ++itr) {
                key = (*itr)->getKey();
                if(loadKey(key) != "") keys.push_back(key);
            }
         }
         break;
         
       case DATA_SELECTED:
          mitr = m_imageList.begin() ;
          for(mitr = m_imageList.begin(); mitr != m_imageList.end(); ++mitr) {
             keys.push_back(mitr->second.c_str());
	  } 
         break;
         
    }
    return keys;
}

set<string> ReviewQueue::getKeyset(int mode, string groupPath) {
    string key;
    set<string> keys;
    list<RQstudy>::iterator sitr;
    list<RQnode>::iterator gitr;
    list<RQnode *>::iterator iitr;
    RQgroup *group;
    ChildList *sl;
    list<RQnode *> *gl;
    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;
    spViewInfo_t view;
    GframeList::iterator gfi;
    list<RQnode *>::iterator itr;
    map<int, string>::iterator mitr;
    switch (mode) {
       case DATA_ALL:
       case DATA_SELECTED_RQ:

         for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
           sl = sitr->getChildren();
           for (gitr=sl->begin(); gitr != sl->end(); ++gitr) {
              group = (RQgroup *)(&(*gitr));
              if (group->getDisplay() != "yes") continue;
		
              group->updateImageList("all");
              gl = group->getImageList();
              for (iitr=gl->begin(); iitr != gl->end(); ++iitr) {
                 keys.insert((*iitr)->getKey());
              }
           }
         }
         if(keys.size() < 1) {
          DataMap *dataMap = DataManager::get()->getDataMap();
          DataMap::iterator pd;
          for (pd = dataMap->begin(); pd != dataMap->end(); ++pd) {
            key = pd->second->getKey();
            keys.insert(key);
	  }
         }
         break;

       case DATA_SELECTED_FRAMES:
         for (gf=gfm->getFirstSelectedFrame(gfi); gf != nullFrame; gf
                =gfm->getNextSelectedFrame(gfi)) {
            if (gfm->isFrameDisplayed(gf) && (view = gf->getFirstView())
                    != nullView)
	       keys.insert(view->imgInfo->getDataInfo()->getKey());
         }
         break;

       case DATA_DISPLAYED:
         for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf
                =gfm->getNextFrame(gfi)) {
            if (gfm->isFrameDisplayed(gf) && (view = gf->getFirstView())
                    != nullView)
	       keys.insert(view->imgInfo->getDataInfo()->getKey());
         }
         break;

       case DATA_GROUP:
         if(groupPath.length() < 1) {
	   groupPath = getActiveKey();
	   groupPath = groupPath.substr(0,groupPath.find_first_of(" "));
         }
         for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
           sl = sitr->getChildren();
           for (gitr=sl->begin(); gitr != sl->end(); ++gitr) {
              group = (RQgroup *)(&(*gitr));
              if (group->getPath() == groupPath) {
                group->updateImageList("all");
                gl = group->getImageList();
                for (iitr=gl->begin(); iitr != gl->end(); ++iitr) {
                 keys.insert((*iitr)->getKey());
                }
	      }
           }
         }
         break;
         
       case DATA_SELECTED_IMAGES:
         makeSelections(0, 0, getSelection());
         if (m_selectedImages.size() > 0) {
            for (itr = m_selectedImages.begin(); itr != m_selectedImages.end(); ++itr) {
                key = (*itr)->getKey();
                if(loadKey(key) != "") keys.insert(key);
            }
         }
         break;
         
       case DATA_SELECTED:
          mitr = m_imageList.begin() ;
          for(mitr = m_imageList.begin(); mitr != m_imageList.end(); ++mitr) {
             keys.insert(mitr->second.c_str());
	  } 
         break;
         
    }
    return keys;
}

// load data to DataMap
string ReviewQueue::loadKey(string key, string id) {
    if (key.length() == 0|| key.find("/", 0) == string::npos)
        return "";
    DataManager *dm = DataManager::get();
    DataMap *dmap = dm->getDataMap();
    DataMap::iterator pd = dmap->find(key);
    if (pd != dmap->end()) {
	if(id != "") pd->second->setImageNumber(id);
        return key;
    } else {
        int orient = getPlaneOrient(key);
        if (orient == -1) {
            key.replace(key.find_first_of(" "), 1, "/");
	    key = dm->loadFile(key.c_str(), false);
	    pd = dmap->find(key);
            if (pd != dmap->end()) {
	       if(id != "") pd->second->setImageNumber(id);
               return key;
	    } else return "";
        } else {
            return "";
        }
    }
}

int ReviewQueue::getNodeMapSize() {
    return nodeMap.size();
}

int ReviewQueue::aipRQcommand(int argc, char *argv[], int retc, char *retv[]) {
    if (argc < 2)
        return proc_complete;

    ReviewQueue *rq = ReviewQueue::get();

    if (argc > 2&& strcasecmp(argv[1], "read") == 0) {
        rq->readNodes(argv[2]);
    } else if (argc > 2&& strcasecmp(argv[1], "loadImage") == 0) {
        int f = 1;
        if(argc>3) f = atoi(argv[3]);
        rq->loadFile(string(argv[2]), "", f, true);
        return proc_complete;
    } else if (argc > 2&& strcasecmp(argv[1], "loadfid") == 0) {
        rq->loadFid(string(argv[2]));
        return proc_complete;
    } else if (argc > 2&& strcasecmp(argv[1], "load") == 0) {
        int f = 1;
        bool show = false;
        string nid = "";
        if (argc > 5) {
            // dropped to graphic area.
            // aipRQcommand('load', path, node, x, y) 
            // need to plus 1 since frame starts from 0.

            nid = argv[3];
            f = 1 + GframeManager::get()->getFrameToStart(atoi(argv[4]), atoi(argv[5]));
            show = true;
        } else if (argc > 4) {
            // aipRQcommand(path, node, f)
            // f=0 to erase current display and start from 1.
            // f=-1 to use first available frame.

            nid = argv[3];
            f = atoi(argv[4]);
            show = true;
        } else if (argc > 3&& strstr(argv[3], " ") != NULL) {
            // aipRQcommand(path, node)

            nid = argv[3];
        } else if (argc > 3) {
            // aipRQcommand(path, f)

            f = atoi(argv[3]);
            show = true;
        }

        int n = rq->loadData(argv[2], nid, f, show);
        if (retc > 0)
            retv[0] = realString((double)n);

        if (retc > 1) {
            int batches = rq->getBatches();
            retv[1] = realString((double)batches);
        }

    } else if (strcasecmp(argv[1], "getSelection") == 0&& retc > 0) {
        retv[0] = newString(rq->getRQSelection().c_str());
    } else if (strcasecmp(argv[1], "reload") == 0) {
        int mode = (int)getReal("reconMode", 1);
        string sel = getString("rqselection", "");
        if (argc > 2)
            mode = atoi(argv[2]);
        if (argc > 3)
            sel = argv[3];
        rq->reloadData(mode, sel);
    } else if (strcasecmp(argv[1], "unselectDisplay") == 0) {
        rq->unselectDisplay();
        rq->updateStudies();
        rq->notifyVJ("update");
    } else if (argc > 2&& strcasecmp(argv[1], "delete") == 0) {
        // aipRQcommand('delete', $node/$path/$key) 
        // this unload the data but does not remove images from RQ
        rq->deleteData(argv[2]);
    } else if (argc > 3&& strcasecmp(argv[1], "remove") == 0) {
        // aipRQcommand('remove', $x, $y)
        // this unload data and removes images from RQ.
        rq->removeImage(atoi(argv[2]), atoi(argv[3]));
    } else if (argc > 2&& strcasecmp(argv[1], "remove") == 0) {
        // aipRQcommand('remove', $node/$path/$key)
        // this unload data and removes images from RQ.
        rq->removeData(argv[2]);
    } else if (argc > 5&& strcasecmp(argv[1], "move") == 0) {
        // aipRQcommand('move', $x1, $y1, $x2, $y2)
        rq->moveImage(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]),
                atoi(argv[5]));
    } else if (argc > 3&& strcasecmp(argv[1], "move") == 0) {
        // aipRQcommand('move', $node1, $node2)
        rq->moveNodes(argv[2], argv[3]);
    } else if (argc > 5&& strcasecmp(argv[1], "copy") == 0) {
        // aipRQcommand('copy', $x1, $y1, $x2, $y2)
        rq->copyImage(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]),
                atoi(argv[5]));
    } else if (argc > 3&& strcasecmp(argv[1], "copy") == 0) {
        // aipRQcommand('copy', $node1, $node2)
        rq->copyNodes(argv[2], argv[3]);
    } else if (argc > 4&& strcasecmp(argv[1], "setvalue") == 0) {
        // aipRQcommand('setvalue', $node, $attribute, $value)
        rq->setGroupAttribute(argv[2], argv[3], argv[4]);
        rq->updateStudies();
        if(strcmp(argv[3],"sort") == 0) rq->display();
    } else if (argc > 4&& strcasecmp(argv[1], "set") == 0) {
        // aipRQcommand('set', $node, $attribute, $value)
        rq->setGroupAttribute(argv[2], argv[3], argv[4]);
        rq->updateStudies();
        rq->notifyVJ("set");
    } else if (argc > 3&& strcasecmp(argv[1], "setvalue") == 0) {
        // aipRQcommand('setvalue', "all", $attribute, $value)
        rq->setGroupAttribute("all", argv[2], argv[3]);
        rq->updateStudies();
    } else if (argc > 3&& strcasecmp(argv[1], "set") == 0) {
        // aipRQcommand('set', "all", $attribute, $value)
        rq->setGroupAttribute("all", argv[2], argv[3]);
        rq->notifyVJ("set");
    } else if (argc > 3&& strcasecmp(argv[1], "get") == 0) {
        // aipRQcommand('get', $node, $attribute):$value
        string str = rq->getNodeAttribute(argv[2], argv[3]);
        if (retc > 0)
            retv[0] = newString(str.c_str());
    } else if (argc > 2&& strcasecmp(argv[1], "get") == 0) {
        // aipRQcommand('get', $name):$value
        string str = rq->getRQvalue(argv[2]);
        if (retc > 0)
            retv[0] = newString(str.c_str());
    } else if (strcasecmp(argv[1], "select") == 0) {
        // aipRQcommand('select', selection, globalSort)
        // selection 'all', 'images', 'frames', 'rq', 'group'
        // default:
        int displayMode = rq->getDisplayMode();
        string selection = rq->getSelection();

        if (argc > 2) {
            if (strcasecmp(argv[2], "all") == 0) {
                displayMode = DATA_ALL;
            } else if (strcasecmp(argv[2], "rq") == 0) {
                displayMode = DATA_SELECTED_RQ;
            } else if (strcasecmp(argv[2], "frames") == 0) {
                displayMode = DATA_SELECTED_FRAMES;
            } else if (strcasecmp(argv[2], "group") == 0) {
                displayMode = DATA_GROUP;
            } else if (strcasecmp(argv[2], "images") == 0) {
                displayMode = DATA_SELECTED_IMAGES;
            } else if (displayMode == DATA_SELECTED_IMAGES) {
                selection = argv[2];
            } else {
		displayMode = atoi(argv[2]);
            }
        }

        rq->selectImages(displayMode);
        if (retc > 0)
            retv[0] = realString((double)rq->getSelSize());

        rq->notifyVJ("setSelection");

    } else if (strcasecmp(argv[1], "display") == 0&& argc > 2&& argv[2][0]
            == '/'&& strstr(argv[2], " ") != NULL) {
        // argv[2] is a nid or key of a group 
        // aipRQcommand('display', key)
        // aipRQcommand('display', key, globalSort, layoutMode)
        // aipRQcommand('display', key, globalSort, layoutMode, firstFrm)
        // aipRQcommand('display', key, firstFrame)
        int globalSort = rq->getGlobalSort();
        int layoutMode = rq->getLayoutMode();
        string selection = rq->getSelection();
        int f = 1;

        bool notify = false;
        if (argc > 6) {
            globalSort = atoi(argv[3]);
            layoutMode = atoi(argv[4]);
            f = 1 + GframeManager::get()->getFrameToStart(atoi(argv[5]), atoi(argv[6]));
            notify = true;
        } else if (argc > 5) {
            globalSort = atoi(argv[3]);
            layoutMode = atoi(argv[4]);
            f = atoi(argv[5]);
            notify = true;
        } else if (argc > 4) {
            globalSort = atoi(argv[3]);
            layoutMode = atoi(argv[4]);
        } else if (argc > 3) {
            f = atoi(argv[3]);
            notify = true;
        }

        if(f < 0) {
            rq->unselectDisplay();
	    f = abs(f);
        }

        rq->displayNode(argv[2], globalSort, layoutMode, f);
        if (retc > 0) {
            int batches = rq->getBatches();
            retv[0] = realString((double)batches);
        }

        string key = argv[2];
        RQnode *node = rq->getNode(key);
        if (node != (RQnode *)NULL && node->getAttribute("type") == "scan") {
             rq->notifyVJ("loadDir");
        }

    } else if (strcasecmp(argv[1], "display") == 0&& argc > 2&& argv[2][0]
            == '/') {
        // argv[2] is a path
        // this is the same as 'load', but images are always displayed.
        int f = 1;
        string nid = "";
        if (argc > 5) {
            nid = argv[3];
            f = 1 + GframeManager::get()->getFrameToStart(atoi(argv[4]), atoi(argv[5]));
        } else if (argc > 4) {
            nid = argv[3];
            f = atoi(argv[4]);
        } else if (argc > 3&& strstr(argv[3], " ") != NULL) {
            nid = argv[3];
        } else if (argc > 3) {
            f = atoi(argv[3]);
        }

        int n = rq->loadData(argv[2], nid, f, true);
        if (retc > 0)
            retv[0] = realString((double)n);

        if (retc > 0) {
            int batches = rq->getBatches();
            retv[0] = realString((double)batches);
        }

    } else if (strcasecmp(argv[1], "display") == 0) {
        // aipRQcommand('display')
        // aipRQcommand('display', selection)
        // aipRQcommand('display', selection, globalSort, layoutMode)
        // aipRQcommand('display', selection, globalSort)
        int displayMode = rq->getDisplayMode();
        int globalSort = rq->getGlobalSort();
        int layoutMode = rq->getLayoutMode();
        string selection = rq->getSelection();

        bool notify = false;
        if (argc > 4) {
            notify = true;
            selection = argv[2];
            globalSort = atoi(argv[3]);
            layoutMode = atoi(argv[4]);
        } else if (argc > 3) {
            notify = true;
            selection = argv[2];
            globalSort = atoi(argv[3]);
        }
        if (argc > 2) {
            notify = true;
            if (strcasecmp(argv[2], "all") == 0) {
                displayMode = DATA_ALL;
            } else if (strcasecmp(argv[2], "rq") == 0) {
                displayMode = DATA_SELECTED_RQ;
            } else if (strcasecmp(argv[2], "rqframes") == 0) {
                displayMode = DATA_SELECTED_RQ;
		rq->displayRQ();
                return proc_complete;
            } else if (strcasecmp(argv[2], "frames") == 0) {
                displayMode = DATA_SELECTED_FRAMES;
            } else if (strcasecmp(argv[2], "group") == 0) {
                displayMode = DATA_GROUP;
            } else if (strcasecmp(argv[2], "selected") == 0) {
                displayMode = DATA_SELECTED;
            } else if (strcasecmp(argv[2], "images") == 0) {
                displayMode = DATA_SELECTED_IMAGES;
            } else if (displayMode == DATA_SELECTED_IMAGES) {
                selection = argv[2];
            } else if(isdigit(argv[2][0])) {
		displayMode = atoi(argv[2]);
            }
        }

        if (rq->getNodeMapSize() == 0) {
            DataManager::get()->displaySelected(displayMode, layoutMode, 1);
            return proc_complete;
        }

        //rq->displaySel(displayMode, globalSort, layoutMode, selection);
        setReal("aipAutoLayout",layoutMode,false); 
        setReal("rqsort",globalSort,false); 
        rq->selectImages(displayMode);
        rq->display();

        if (retc > 0) {
            int batches = rq->getBatches();
            retv[0] = realString((double)batches);
        }

        if (notify)
            rq->notifyVJ("setSelection");

    } else if (strcasecmp(argv[1], "displayBatch") == 0) {
        int batch = rq->getCurrentBatch();
        int batches = rq->getBatches();
        if (argc > 2) {
            if (strcasecmp(argv[2], "first") == 0) {
                batch = 1;
            } else if (strcasecmp(argv[2], "last") == 0) {
                batch = batches;
            } else if (strcasecmp(argv[2], "next") == 0) {
                if (batch >= batches || batch < 1)
                    batch = 1;
                else
                    batch = batch + 1;
            } else if (strcasecmp(argv[2], "prev") == 0) {
                if (batch <= 1|| batch > batches)
                    batch = batches;
                else
                    batch = batch - 1;
            } else {
                batch = atoi(argv[2]);
            }
        }
        if (rq->getNodeMapSize() == 0) {
            int displayMode = rq->getDisplayMode();
            int layoutMode = rq->getLayoutMode();
            DataManager::get()->displaySelected(displayMode, layoutMode, batch);
            return proc_complete;
        }

        rq->display(batch);
        if (retc > 0) {
            int batches = rq->getBatches();
            retv[0] = realString((double)batches);
        }
    } else if (strcasecmp(argv[1], "nameFrames") == 0) {
        DataManager::get()->makeShortNames();
        DataManager::get()->numberData();
        rq->display(rq->getCurrentBatch());
    }
    return proc_complete;
}

string ReviewQueue::getActiveKey() {
    GframeManager* gfm = GframeManager::get();
    GframeList::iterator gfi;
    spGframe_t gf = gfm->getActiveGframe();
    if(gf == nullFrame) { 
      gf = gfm->getFirstSelectedFrame(gfi);
    }
    if (gf == nullFrame)
       gf = gfm->getFirstFrame(gfi);
    if (gf == nullFrame)
        return "";

    spImgInfo_t img = gf->getSelImage();
    if(img == nullImg) return "";
    else return img->getDataInfo()->getKey();
}

void ReviewQueue::makeImageList_Group() {
    m_selectedImages.clear();
    m_groups.clear();
    //unselectDisplay();
    RQnode *node = getNode(getActiveKey());
    if (node == (RQnode *)NULL)
        return;
    RQgroup *group = (RQgroup *)node->getParent();
    if (group == (RQgroup *)NULL)
        return;

    group->updateImageList("all");
    //group->setAttribute("display","yes");
    m_groups.push_back(group);
    list<RQnode *> *l = group->getImageList();
    list<RQnode *>::iterator itr;
    for (itr = l->begin(); itr != l->end(); ++itr)
        m_selectedImages.push_back(*itr);

}

void ReviewQueue::makeImageList_RQ(int displayMode, int globalSort,
        int layoutMode, string selection) {
    m_selectedImages.clear();
    m_groups.clear();
    char str[16];

    switch (displayMode) {
    default:
    case DATA_ALL:
        strcpy(str, "all");
        break;
    case DATA_SELECTED_RQ:
        strcpy(str, "selected");
        break;
    case DATA_SELECTED_IMAGES:
        setSelection(selection);
        strcpy(str, "selected");
        break;
    }

    list<RQstudy>::iterator sitr;
    list<RQnode>::iterator gitr;
    ChildList *gl;
    RQgroup *group;

    for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
        gl = sitr->getChildren();
        for (gitr=gl->begin(); gitr != gl->end(); ++gitr) {
            group = (RQgroup *)(&(*gitr));
            if (strcasecmp(str, "all") != 0&& group->getDisplay() != "yes")
                continue;
            //group->setAttribute("display", "yes");
            group->updateImageList(string(str));
            m_groups.push_back(group);
        }
    }
    if (layoutMode == -1|| (globalSort > 0&& m_groups.size() > 0))
        makeImageList_RQ(globalSort);
}

void ReviewQueue::makeImageList_RQ(int globalSort) {
    list<RQgroup *>::iterator gitr;
    list<RQnode *>::iterator iitr;
    list<RQnode *> *l;
    if (globalSort < 2|| m_groups.size() == 1) {// sort by scans 
        for (gitr=m_groups.begin(); gitr != m_groups.end(); ++gitr) {
            l = (*gitr)->getImageList();
            for (iitr=l->begin(); iitr != l->end(); ++iitr) {
                m_selectedImages.push_back(*iitr);
            }
        }
    } else { // sort by slices
        unsigned int msize = 0;
        vector<vector<RQnode *> > images2D;
        vector<RQnode *> images1D;
        for (gitr=m_groups.begin(); gitr != m_groups.end(); ++gitr) {
            l = (*gitr)->getImageList();
            images1D.clear();
            for (iitr=l->begin(); iitr != l->end(); ++iitr) {
                images1D.push_back(*iitr);
            }
            if (images1D.size() > msize)
                msize = images1D.size();
            images2D.push_back(images1D);
        }
        if (images2D.size() <= 0)
            return;

        for (int i=0; i<msize; i++) {
            for (int j=0; j<images2D.size(); j++) {
                if (i < images2D[j].size())
                    m_selectedImages.push_back(images2D[j][i]);
            }
        }
    }
}

void ReviewQueue::makeImageList(int displayMode, int globalSort,
        int layoutMode, string selection) {

    if (m_dragging == 1)
        m_dragging = 2;

    list<string> keylist;
    list<string>::iterator itr;

    switch (displayMode) {
    default:
    case DATA_ALL:
    case DATA_SELECTED_RQ:
        makeImageList_RQ(displayMode, globalSort, layoutMode, selection);
        break;
    case DATA_SELECTED_IMAGES:
        makeSelections(globalSort, layoutMode, selection);
        break;
    case DATA_GROUP:
        makeImageList_Group();
        break;
    case DATA_SELECTED_FRAMES:
        if (layoutMode == -1) {
            m_groups.clear();
            m_selectedImages.clear();
        }
        if (WinMovie::get()->getGuestFrame() == -1&& WinMovie::get()->getOwnerFrame() == -1)
            keylist = GframeManager::get()->getSelectedKeylist();
        if (keylist.size() > 0) {
            m_groups.clear();
            m_selectedImages.clear();
            for (itr = keylist.begin(); itr != keylist.end(); ++itr) {
                RQnode * node = getNode(*itr);
                if (node != (RQnode *)NULL)
                    m_selectedImages.push_back(node);
            }
        }
        break;
    case DATA_DISPLAYED:
        m_groups.clear();
        m_selectedImages.clear();
        GframeManager *gfm = GframeManager::get();
        spGframe_t gf;
        spViewInfo_t view;
        GframeList::iterator gfi;
        for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf
                =gfm->getNextFrame(gfi)) {
            if (!gfm->isFrameDisplayed(gf) ||(view = gf->getFirstView())
                    == nullView)
                continue;

            RQnode * node = getNode(view->imgInfo->getDataInfo()->getKey());
            if (node != (RQnode *)NULL)
                m_selectedImages.push_back(node);
        }
        break;
    }
}

void ReviewQueue::makeSelections(int globalSort, int layoutMode,
        string selection) {
    m_selectedImages.clear();
    m_groups.clear();

    m_batches = 1;
    RQparser *ps = RQparser::get();
    list<string> sl;
    if (selection.find("coil", 0) == string::npos)
        sl = ps->parseStatement(selection);
    else
        sl.push_back("NA "+selection+" NA NA NA 1-");

    list<RQstudy>::iterator sitr;
    list<RQnode>::iterator gitr;
    list<RQnode>::iterator iitr;
    list<int>::iterator pt;
    ChildList *gl;
    RQgroup *group;

    string gstr="";
    list<string>::iterator itr;
    for (itr = sl.begin(); itr != sl.end(); ++itr) {
        int p1;
        if ((p1 = itr->find_first_of(" ")) != string::npos)
            gstr = itr->substr(0, p1);

        p1 = p1 + 1;
        int p2 = itr->find(" ", p1);
        int p3 = p2;
        p3 = itr->find(" ", p3+1); //skip slices
        p3 = itr->find(" ", p3+1); //skip echoes
        p3 = itr->find(" ", p3+1); //skip array
        string images = itr->substr(p1, p2-p1);
        string frames = itr->substr(p3+1);

        list<int> sel = ps->parseGroups(gstr, getNumGroups());
        if (sel.size() <= 0) {
            // global selection if group is not specified.
            makeGlobalSelection(globalSort, images);
            return;
        }

        int ng = 0;
        bool b = false;
        bool found = false;
        for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
            gl = sitr->getChildren();
            for (gitr=gl->begin(); gitr != gl->end(); ++gitr) {
                ng++;

                found = false;
                for (pt = sel.begin(); pt != sel.end(); ++pt) {
                    if (ng == (*pt)) {
                        found = true;
                        break;
                    }
                }

                group = (RQgroup *)(&(*gitr));
                if (!found) {
                    if (group->getAttribute("display") == "yes") {
                        b = true;
                        group->setAttribute("display", "no");
                    }
                } else {
                    group->setSelection(*itr);
                    if (group->getAttribute("display") == "no") {
                        b = true;
                        group->setAttribute("display", "yes");
                    }
                    group->updateImageList("selected");

                    if (frames.length() > 0&& frames != "NA")
                        globalSort = 0;
                    m_groups.push_back(group);
                }
                // need to change m_groups to list<RQgroup> and get the last element to set
                // the selection if don't want to change what is selected in RQ.
            }
        }

        //if(b) notifyVJ("notify");
        notifyVJ("notify");
    }

    if (layoutMode == -1|| (globalSort > 0&& m_groups.size() > 0)) {
        makeImageList_RQ(globalSort);
    }
}

string ReviewQueue::getFirstSelectedImage() {
    if (m_imageList.size() > 0) {
        return m_imageList.begin()->second;
    } else if (m_groups.size() > 0) {
        ChildList *il = (*m_groups.begin())->getChildren();
        if (il->size() > 0)
            return il->begin()->getKey();
        else
            return string("");
    } else
        return string("");
}

double ReviewQueue::getAspectRatio() {
    double aspect = 1.0;
    string key = loadKey(getFirstSelectedImage());
    if (key.length() > 0) {
        DataMap *dmap = DataManager::get()->getDataMap();
        double spans[3];
        DataMap::iterator pd = dmap->find(key);
        if (pd == dmap->end())
            return aspect;

        pd->second->getSpatialSpan(spans);
        if (spans[0] * spans[1]!= 0) {
            aspect = spans[0] / spans[1];
        }
        int rotation = WinRotation::calcRotation(pd->second);
        if (rotation & 4) {
            aspect = 1 / aspect; // Image gets transposed for display
        }
    }
    return aspect;
}

void ReviewQueue::setSelection(string str) {
    // cannot do global selection in this method.
    RQparser *ps = RQparser::get();
    list<string> l = ps->parseStatement(str);

    string gstr="1";
    list<string>::iterator itr;
    for (itr = l.begin(); itr != l.end(); ++itr) {
        str = *itr;
        cout << str << endl;

        int p1;
        if ((p1 = str.find_first_of(" ")) != string::npos)
            gstr = str.substr(0, p1);

        list<int> sel = ps->parseGroups(gstr, getNumGroups());
        if (sel.size() > 0) {
            list<int>::iterator pt;
            for (pt = sel.begin(); pt != sel.end(); ++pt) {
                setGroupSelection(*pt, str);
            }
        }
    }
}

void ReviewQueue::setGroupSelection(int gid, string str) {
    list<RQstudy>::iterator itr;
    list<RQnode>::iterator gitr;
    ChildList *gl;
    RQgroup *group;

    for (itr = studies->begin(); itr != studies->end(); ++itr) {
        gl = itr->getChildren();
        for (gitr=gl->begin(); gitr != gl->end(); ++gitr) {
            group = (RQgroup *)(&(*gitr));
            if (gid == group->getGnum()) {
                group->setSelection(str);
                group->setAttribute("display", "yes");
                return;
            }
        }
    }
}

string ReviewQueue::getKey(string str) {
    if (str.find("/", 0) == 0&& str.find(" ", 0) == string::npos) {
        // str is path
        str = str.replace(str.find_last_of("/"), 1, " ") + " 0";
        return str;
    } else if (str.find("/", 0) == 0) {
        // str is key
        return str;
    } else if (str.find(" ", 0) == string::npos) {
        const char *s = str.c_str();
        if (s[0] == 'G'|| s[0] == 'g') {
            // str is group id or number 
            RQgroup * group = getRQgroup(getGnum(str));
            if (group != (RQgroup *)NULL)
                return group->getKey();
            else
                return "";
        } else
            return "";
    } else {// str is nid
        RQnode * node = getRQnode("nid", str);
        if (node != (RQnode *)NULL)
            return node->getKey();
        else
            return "";
    }
}

void ReviewQueue::removeData(string str) {
    deleteData(str);
    m_imageList.clear();

    if (strcasecmp(str.c_str(), "all") == 0) {
        studies->clear();
        m_selectedImages.clear();
        m_groups.clear();
        m_batches = 1;
        m_batch = 1;
        setReal("aipBatches", m_batches, true);
        setReal("aipBatch", m_batch, true);
        updateStudies();
        notifyVJ("removeNodes");
        return;
    }

    if (strcasecmp(str.c_str(), "sel") == 0) {

        /*
         list<RQnode *> il = getSelectedFrames();

         string key;
         list<RQnode *>::iterator iitr;
         for(iitr = il.begin(); iitr != il.end(); ++iitr) {
         key = (*iitr)->getKey();
         removeNode((RQimage *)getNode(key));
         }

         */
        list<string> il = GframeManager::get()->getSelectedKeylist();
        list<string>::iterator iitr;
        int i = 0;
        for (iitr = il.begin(); iitr != il.end(); ++iitr) {
            RQnode *node = getNode(*iitr);
            if (node != (RQnode *)NULL) {
                removeNode((RQimage *)node);
		
                i++;
            }
        }

    } else if (strcasecmp(str.c_str(), "selected") == 0) {

        list<RQnode *>::iterator itr;
        if (m_selectedImages.size() > 0) {
            for (itr = m_selectedImages.begin(); itr != m_selectedImages.end(); ++itr) {
                removeNode((RQimage *)(*itr));
            }
        } else if (m_groups.size() > 0) {
            list<RQnode *> *lst;
            list<RQgroup *>::iterator gitr;
            for (gitr = m_groups.begin(); gitr != m_groups.end(); ++gitr) {

                lst = (*gitr)->getImageList();
                for (itr = lst->begin(); itr != lst->end(); ++itr) {
                    removeNode((RQimage *)(*itr));
                }
            }
        }
    } else {
        RQnode *node = getNode(str);

        if (node == (RQnode *)NULL)
            return;

        string type = node->getAttribute("type");

        if (type == "study") {
            removeNode((RQstudy *)node);
        } else if (type == "scan") {
            removeNode((RQgroup *)node);
        } else if (type == "img") {
            removeNode((RQimage *)node);
        }
    }
    updateStudies();
    selectImages(DATA_SELECTED_RQ);

    WinMovie::dataMapChanged();
    notifyVJ("removeNodes");
}

void ReviewQueue::copyNodes(string nid1, string nid2) {
    RQnode *node = getNode(nid1);

    if (node == (RQnode *)NULL)
        return;

    // cannot copy 3D data for now.
    string key = node->getKey();
    if (node->getAttribute("rank") != "2"||key.find(".fdf_xy", 1)
            != string::npos ||key.find(".fdf_xz", 1) != string::npos
            ||key.find(".fdf_yz", 1) != string::npos )
        return;

    string type = node->getAttribute("type");

    if (type == "study") {
        copyNode((RQstudy *)node, nid2);
    } else if (type == "scan") {
        copyNode((RQgroup *)node, nid2);
    } else if (type == "img") {
        copyNode((RQimage *)node, nid2);
    }

    updateStudies();
    notifyVJ("copyNodes");
}

void ReviewQueue::moveNodes(string nid1, string nid2) {
    RQnode *node = getNode(nid1);

    if (node == (RQnode *)NULL)
        return;

    // cannot move 3D data for now.
    /*
     string key = node->getKey();
     if(node->getAttribute("rank") != "2" || 
     key.find(".fdf_xy", 1) != string::npos ||
     key.find(".fdf_xz", 1) != string::npos ||
     key.find(".fdf_yz", 1) != string::npos ) return;
     */

    string type = node->getAttribute("type");

    if (type == "study") {
        moveNode((RQstudy *)node, nid2);
    } else if (type == "scan") {
        moveNode((RQgroup *)node, nid2);
    } else if (type == "img") {
        moveNode((RQimage *)node, nid2);
    }

    updateStudies();
    notifyVJ("moveNodes");
}

string ReviewQueue::getNodeAttribute(string str, string name) {
    RQnode *node = getNode(str);

    if (node == (RQnode *)NULL)
        return "";

    string value = node->getAttribute(name);
    if (value == ""&& node->getAttribute("type") == "scan") {
        double d = 1;
        RQgroup::getRealFromProcpar(node->getPath(), &name, &d, 1);
        char v[MAXSTR];
        sprintf(v, "%f", d);
        return string(v);
    } else if (value == ""&& node->getAttribute("type") == "img") {
        string v = DataManager::get()->getHeaderParam(node->getKey(), name);
        int p = v.find(" ", 0);
        if (p != string::npos) {
            value = v.substr(p+1, v.length()-1);
            return value;
        }
    }
    return value;
}

string ReviewQueue::getRQvalue(string name) {
    char str[MAXSTR];
    strcpy(str, "");
    if (strcasecmp(name.c_str(), "numofimages") == 0)
        sprintf(str, "%d", getNumImages());
    else if (strcasecmp(name.c_str(), "numofgroups") == 0)
        sprintf(str, "%d", getNumGroups());
    else if (strcasecmp(name.c_str(), "numofstudies") == 0)
        sprintf(str, "%d", studies->size());
    else if (strcasecmp(name.c_str(), "batches") == 0)
        sprintf(str, "%d", m_batches);
    else if (strcasecmp(name.c_str(), "batch") == 0)
        sprintf(str, "%d", m_batch);
    else if (strcasecmp(name.c_str(), "selectedImages") == 0) {
        int ni = m_imageList.size();
        if (ni == 0) {
            ChildList::iterator gitr;
            ChildList *gl;
            list<RQstudy>::iterator sitr;
            for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
                gl = sitr->getChildren();
                for (gitr=gl->begin(); gitr != gl->end(); ++gitr) {
                    if (gitr->getAttribute("display") == "yes")
                        ni += gitr->getChildrenCount();
                }
            }
        }
        sprintf(str, "%d", ni);
    } else if (strcasecmp(name.c_str(), "selectedGroups") == 0) {
        int ni = m_groups.size();
        if (ni == 0) {
            ChildList::iterator gitr;
            ChildList *gl;
            list<RQstudy>::iterator sitr;
            for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
                gl = sitr->getChildren();
                for (gitr=gl->begin(); gitr != gl->end(); ++gitr) {
                    if (gitr->getAttribute("display") == "yes")
                        ni++;
                }
            }
        }
        sprintf(str, "%d", ni);
    }
    return string(str);
}

void ReviewQueue::setGroupAttribute(string str, string val, string sel) {
    if (strcasecmp(str.c_str(), "all") == 0) {
        ChildList *gl;
        ChildList::iterator gitr;
        list<RQstudy>::iterator sitr;
        for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
            gl = sitr->getChildren();
            for (gitr = gl->begin(); gitr != gl->end(); ++gitr) {
                setGroupAttribute(gitr->getKey(), val, sel);
            }
        }
        return;
    }

    RQgroup *group = getGroup(str);

    if (group == (RQgroup *)NULL)
        return;

    if (strcasecmp(val.c_str(), "images") == 0)
        group->setImageSelection(sel);
    else if (strcasecmp(val.c_str(), "slices") == 0)
        group->setSliceSelection(sel);
    else if (strcasecmp(val.c_str(), "echoes") == 0)
        group->setEchoSelection(sel);
    else if (strcasecmp(val.c_str(), "array") == 0)
        group->setArraySelection(sel);
    else if (strcasecmp(val.c_str(), "frames") == 0)
        group->setFrameSelection(sel);
    else if (strcasecmp(val.c_str(), "sort") == 0) {
        m_remakeImageList = true;
        group->sortImages(sel);
    } else if (strcasecmp(val.c_str(), "display") == 0) {
        if (strcasecmp(sel.c_str(), "yes") == 0)
            group->setAttribute("display", "yes");
        else
            group->setAttribute("display", "no");
    } else if (strcasecmp(val.c_str(), "expand") == 0) {
        if (strcasecmp(sel.c_str(), "yes") == 0)
            group->setAttribute("expand", "yes");
        else
            group->setAttribute("expand", "no");
    } else {
        group->setAttribute(val, sel);
    }
}

RQgroup *ReviewQueue::getGroup(string str) {

    return getRQgroup("key", getKey(str));
}

RQnode *ReviewQueue::getNode(string str) {

    return getRQnode(getKey(str));
}

void ReviewQueue::makeGlobalSelection(int globalSort, string str) {
    m_selectedImages.clear();
    m_groups.clear();

    list<RQgroup *> groups;
    list<RQgroup *>::iterator gitr;
    map<int, RQnode *> imap;

    list<RQstudy>::iterator sitr;
    list<RQnode *> *l;
    list<RQnode>::iterator iitr;
    ChildList *gl;
    RQgroup *group;
    bool b = false;
    int id = 0;
    int pos;
    for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
        gl = sitr->getChildren();
        for (iitr=gl->begin(); iitr != gl->end(); ++iitr) {
            group = (RQgroup *)(&(*iitr));
            if (str == "1-"|| strcasecmp(str.c_str(), "all") == 0) {
                if (group->getAttribute("display") == "no") {
                    b=true;
                    group->setAttribute("display", "yes");
                }
            } else if ((pos = str.find("coil", 0)) != string::npos) {
                id = atoi(str.substr(pos+4).c_str());
                group->selectByPar("coil", id);
                if (group->getSelSize() == 0)
                    group->updateImageList("all");
            }
            //group->setFrameSelection("NA");
            groups.push_back(group);
        }
    }

    if (b)
        notifyVJ("notify");

    if (groups.size() < 1)
        return;
    else if (groups.size() == 1&& (str == "1-"|| strcasecmp(str.c_str(), "all")
            == 0)) {
        m_groups.push_back(*(groups.begin()));
        (*(m_groups.begin()))->updateImageList("all");
        return;
    }

    if (id > 0) {
        for (gitr=groups.begin(); gitr != groups.end(); ++gitr) {
            m_groups.push_back(*gitr);
        }
        return;
    }

    list<RQnode *>::iterator iitr2;

    if (globalSort < 2|| groups.size() == 1) { // sort by scans
        int n = 0;
        for (gitr=groups.begin(); gitr != groups.end(); ++gitr) {
            (*gitr)->updateImageList("all");
            l = (*gitr)->getImageList();
            for (iitr2=l->begin(); iitr2 != l->end(); ++iitr2) {
                n++;
                imap.insert(map<int, RQnode *>::value_type(n, *iitr2));
            }
        }
    } else { // Sort by slices
        int msize = 0;
        vector<vector<RQnode *> > images2D;
        vector<RQnode *> images1D;
        for (gitr=groups.begin(); gitr != groups.end(); ++gitr) {
            (*gitr)->updateImageList("all");
            l = (*gitr)->getImageList();
            images1D.clear();
            for (iitr2=l->begin(); iitr2 != l->end(); ++iitr2) {
                images1D.push_back(*iitr2);
            }
            if (images1D.size() > msize)
                msize = images1D.size();
            images2D.push_back(images1D);
        }
        if (images2D.size() <= 0)
            return;

        int n = 0;
        for (int i=0; i<msize; i++) {
            for (int j=0; j<images2D.size(); j++) {
                if (i < images2D[j].size()) {
                    n++;
                    imap.insert(map<int, RQnode *>::value_type(n,
                            images2D[j][i]));
                }
            }
        }
    }

    int ni = getNumImages();
    list<int> nlist = RQparser::get()->parseImages(str, ni);
    list<int>::iterator nitr;
    map<int, RQnode *>::iterator mitr;

    for (nitr = nlist.begin(); nitr != nlist.end(); ++nitr) {
        mitr = imap.find(*nitr);
        if (mitr != imap.end()) {
            m_selectedImages.push_back(mitr->second);
        }
    }
}

void ReviewQueue::displayData(int batch) {
    display(batch);
}

void ReviewQueue::displayBatch(int cmdbits) {
    int batchNext = 2;
    int batchPrevious = 4;
    int batchFirst = 8;
    int batchLast = 16;

    int batch = m_batch;
    if (cmdbits & batchFirst) batch=1;
    if (cmdbits & batchLast) batch=m_batches;
    if (cmdbits & batchNext) batch++;
    if (cmdbits & batchPrevious) batch--;
    display(batch);
}

void ReviewQueue::displaySel(int displayMode, int globalSort, int layoutMode,
        string selection) {

    if(getGlobalSort() != globalSort) setReal("rqsort",globalSort,true); 
    if(getLayoutMode() != layoutMode) setReal("aipAutoLayout",layoutMode,true); 
    if(displayMode == DATA_SELECTED_IMAGES) setString("userselection",selection,true);

    selectImages(displayMode);
    display();
}

void ReviewQueue::displayNode(string str, int sort, int layout, int frame) {
    RQnode *node = getNode(str);
    if (node == (RQnode *)NULL)
        return;

    if (node->getAttribute("type") == "img") {
        int nimages = m_imageList.size();
        m_imageList.insert(map<int, string>::value_type(nimages,str));
	selectImages(DATA_SELECTED_RQ);
        display();
    } else if (node->getAttribute("type") == "scan") {
        node->setAttribute("display", "yes");
        if (frame > 0) {
            char fstr[MAXSTR];
            sprintf(fstr, "%d-", frame);
            node->setAttribute("frames", fstr);
        }
	selectImages(DATA_SELECTED_RQ);
        display();
    }
}

void ReviewQueue::removeExtractedNodes() {
        list<RQstudy>::iterator itr;
        list<RQnode>::iterator gitr;
        list<RQnode>::iterator iitr;
        list<RQnode> *gl;
        string key;
        for (itr = studies->begin(); itr != studies->end(); ++itr) {
            gl = itr->getChildren();
            for (gitr=gl->begin(); gitr != gl->end(); ++gitr) {
                key = gitr->getKey();
                if (key.find("_xy") != string::npos ||
		    key.find("_xz") != string::npos ||
		    key.find("_yz") != string::npos ) {
                    removeNode((RQgroup *)(&(*gitr)));
                    gitr = gl->begin();
                } 
            }
        }
}

void ReviewQueue::deleteData(string str) {
    if (strcasecmp(str.c_str(), "all") == 0) {
        // remove extrated node
        removeExtractedNodes();
        DataManager::get()->deleteAllData();
	unselectDisplay();
        updateStudies();
        notifyVJ("deleteData");
        VolData::get()->showObliquePlanesPanel(false);
        return;
    } else if (strcasecmp(str.c_str(), "sel") == 0) {
        DataManager::get()->deleteSelectedData();
        return;
    }

    list<RQnode *> il;
    list<RQnode *>::iterator itr;
    if (strcasecmp(str.c_str(), "selected") == 0) {
        if (m_selectedImages.size() > 0) {
            for (itr = m_selectedImages.begin(); itr != m_selectedImages.end(); ++itr) {
                il.push_back(*itr);
            }
        } else if (m_groups.size() > 0) {
            list<RQnode *> *lst;
            list<RQgroup *>::iterator gitr;
            for (gitr = m_groups.begin(); gitr != m_groups.end(); ++gitr) {

                lst = (*gitr)->getImageList();
                for (itr = lst->begin(); itr != lst->end(); ++itr) {
                    il.push_back(*itr);
                }
            }
        }
    } else {
        RQnode *node = getNode(str);

        if (node == (RQnode *)NULL)
            return;

        il = getImages4Node(node);
    }

    DataMap::iterator pd;
    DataMap *dataMap = DataManager::get()->getDataMap();
    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;

    string key;
    list<RQnode *>::iterator iitr;
    for (iitr = il.begin(); iitr != il.end(); ++iitr) {
        key = (*iitr)->getKey();
        pd = dataMap->find(key);
        if (pd != dataMap->end()) {
            gf = gfm->getCachedFrame(key);
            gfm->unsaveFrame(key);
            if (gf != nullFrame) {
                gf->clearFrame();
                if (aipHasScreen() && gfm->isFrameDisplayed(gf)) {
                    gf->draw();
                }
            }
            dataMap->erase(key);
        }
    }

    WinMovie::dataMapChanged();
    RoiStat::get()->calculate();
    VsInfo::setVsHistogram();
}

list<RQnode *> ReviewQueue::getSelectedFrames() {
    list<RQnode *> l;
    RQnode *node;
    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;
    GframeList::iterator gfi;
    for (gf=gfm->getFirstSelectedFrame(gfi); gf != nullFrame; gf
            =gfm->getNextSelectedFrame(gfi)) {
        spViewInfo_t vi;
        if ((vi=gf->getFirstView()) != nullView) {
            string key = vi->imgInfo->getDataInfo()->getKey();
            node = getNode(key);
            if (node != (RQnode *)NULL) {
                l.push_back(node);
            }
        }
    }
    return l;
}

list<RQnode *> ReviewQueue::getImages4Node(RQnode *node) {
    list<RQnode *> l;
    if (node->getAttribute("type") == "img") {
        l.push_back(node);
    } else if (node->getAttribute("type") == "scan") {
        list<RQnode> *il = node->getChildren();
        list<RQnode>::iterator iitr;
        for (iitr = il->begin(); iitr != il->end(); ++iitr) {
            l.push_back(&(*iitr));
        }
    } else {
        list<RQnode> *gl = node->getChildren();
        list<RQnode>::iterator gitr;
        for (gitr = gl->begin(); gitr != gl->end(); ++gitr) {
            list<RQnode> *il = gitr->getChildren();
            list<RQnode>::iterator iitr;
            for (iitr = il->begin(); iitr != il->end(); ++iitr) {
                l.push_back(&(*iitr));
            }
        }
    }

    return l;
}

int ReviewQueue::getDisplayMode() {
    return (int)getReal("aipDisplayMode", DATA_SELECTED_RQ);
}

int ReviewQueue::getGlobalSort() {
    return (int)getReal("rqsort", 1);
}

int ReviewQueue::getLayoutMode() {
    return (int)getReal("aipAutoLayout", 0);
}

string ReviewQueue::getSelection() {
    return getString("userselection", "");
}

void ReviewQueue::reloadData(int mode, string selection) {
    if (mode == DATA_ALL) {
	DataManager::get()->deleteAllData();
        ChildList *gl;
        ChildList::iterator gitr;
        list<RQstudy>::iterator sitr;
        for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
            gl = sitr->getChildren();
            for (gitr = gl->begin(); gitr != gl->end(); ++gitr) {
                ((RQgroup *)(&(*gitr)))->resetChildren();
            }
        }
    } else if (mode == DATA_SELECTED_RQ) {
        ChildList *gl;
        ChildList::iterator gitr;
        list<RQstudy>::iterator sitr;
        for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
            gl = sitr->getChildren();
            for (gitr = gl->begin(); gitr != gl->end(); ++gitr) {
                if (gitr->getAttribute("display") == "yes") {
                    deleteData(gitr->getKey());
                    ((RQgroup *)(&(*gitr)))->resetChildren();
                }
            }
        }
    } else if (mode == DATA_GROUP) {
      RQnode *node = getNode(getActiveKey());
      if (node == (RQnode *)NULL) {
	deleteData(node->getKey());
        ((RQgroup *)node)->resetChildren();
      }
    } else if (mode == DATA_SELECTED) {
        deleteData("selected");
    } else if (mode == DATA_SELECTED_FRAMES) {
        deleteData("sel");
    }

    if (mode == DATA_ALL|| mode == DATA_SELECTED_RQ) {
        updateStudies();
        notifyVJ("reloadData");
    }
    selectImages(mode);
    display();
}

void ReviewQueue::moveImage(int x1, int y1, int x2, int y2) {
    spGframe_t gf =GframeManager::get()->getGframeFromCoords(x1, y1);
    string key = "";
    spViewInfo_t vi;
    if (gf != nullFrame && (vi=gf->getFirstView()) != nullView)
        key = vi->imgInfo->getDataInfo()->getKey();

    int f = 1 +GframeManager::get()->getFrameToStart(x2, y2);

    if (key != ""&& f > 0)
        displayNode(key, 1, 0, f);
}

void ReviewQueue::copyImage(int x1, int y1, int x2, int y2) {
    spGframe_t gf =GframeManager::get()->getGframeFromCoords(x1, y1);
    string key = "";
    spViewInfo_t vi;
    if (gf != nullFrame && (vi=gf->getFirstView()) != nullView)
        key = vi->imgInfo->getDataInfo()->getKey();

    int f = 1 +GframeManager::get()->getFrameToStart(x2, y2);

    if (key != ""&& f > 0) {
        copyNodes(key, "");
        displayNode(studies->back().getChildren()->back().getChildren()->back().getKey(), 1, 0, f);
    }
}

void ReviewQueue::removeImage(int x, int y) {
    spGframe_t gf =GframeManager::get()->getGframeFromCoords(x, y);
    string key = "";
    spViewInfo_t vi;
    if (gf != nullFrame && (vi=gf->getFirstView()) != nullView)
        key = vi->imgInfo->getDataInfo()->getKey();

    if (key != "") {
        removeData(key);
    }
}

string ReviewQueue::getShortName(string key, int type) {
    RQimage *image = (RQimage *)(getNode(key));

    if (image == (RQimage *)NULL)
        return "";

    if (type == 3)
        return image->getAttribute("lindex");
    else if (type == 2)
        return image->getAttribute("gindex");
    else
        return image->getAttribute("shortName");
}

// this is called by extract_oblplanes to display the 3 extracted planes.
void ReviewQueue::disImagePlane(spDataInfo_t dataInfo, string key, int n,
        int slice, int first, int last) {

   GframeManager *gfm = GframeManager::get();
   int pos;
   int ind=0;
   if((pos = key.find("xy")) != string::npos) {
      gfm->setFrameToLoad(0);
      ind=1;
   } else if((pos = key.find("yz")) != string::npos) {
      gfm->setFrameToLoad(1);
      ind=2;
   } else if((pos = key.find("xz")) != string::npos) {
      gfm->setFrameToLoad(2);
      ind=3;
   } else return;

   string shortName = key.substr(pos, key.find("_",pos+3)-pos);
   
   DataMap *dmap = DataManager::get()->getDataMap();
   DataMap::iterator pd;
   pd = dmap->find(key);
   if (pd != dmap->end()) {
        pd->second->setShortName(shortName);
        pd->second->setImageNumber(shortName);
        pd->second->setDataNumber(ind);
        gfm->loadData(pd->second);
   }
}

// n is number of plane to add (should always be 1).
// slice is index in a group
// first is first index of the group
// last is last index
void ReviewQueue::addImagePlane(spDataInfo_t dataInfo, string key, int n,
        int slice, int first, int last) {
    RQimage image = RQimage(key);

    int orient = getPlaneOrient(key);
    if (orient == -1)
        return;

    string gpath = getGroupPath(key);

    int ns = 1;
    DDLSymbolTable *st = dataInfo->st;
    if (st && n == 1) {
        st->GetValue("matrix", ns, orient);
    }
    char str[64];
    sprintf(str, "%d", slice+1);
    image.setAttribute("ns",string(str));

    sprintf(str, "%d", ns);
    string lastStr = string(str);

//Winfoprintf("add image %d %d %d %d %d %s %s",n,slice,first,last,ns,key.c_str(),gpath.c_str());
    ChildList *gl;
    ChildList::iterator gitr;
    list<RQstudy>::iterator sitr;
    int ff = 1;
    RQgroup *group;
    for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
        gl = sitr->getChildren();
        for (gitr = gl->begin(); gitr != gl->end(); ++gitr) {
            group = ((RQgroup *)(&(*gitr)));
            if (group->getPath() == gpath) {
                if (n == 1&& slice == first) {
                    group->getChildren()->clear();
                }
                group->addChild(image);
                if (n == 1&& slice == last) {
                    group->setAttribute("display", "yes");
	            sprintf(str, "%d-", ff);
                    group->setAttribute("frames", string(str));
    		    sprintf(str, "%d-%d", 1,last-first+1);
    		    group->setAttribute("images", string(str));
    		    sprintf(str, "%d", last-first+1);
    		    group->setAttribute("ns", string(str));
    		    updateStudies();
    		    notifyVJ("loadFile");
                }
                return;
            }
            if (group->getAttribute("display") == "yes") {
                int size = group->getSelSize();
                if (size == 0)
                    size = group->getChildrenCount();
                ff += size;
            }
        }
    }
    sprintf(str, "%d-", ff);
    string frame = string(str);

    /* 
     string line = "<filenode array=\"1\" copy=\" 0\" dir=\""+dir+"\" display=\"yes\" echoes=\"1\" expand=\"no\" ext=\""+ext+"\" frames=\""+frame+"\" images=\"1-"+lastStr+"\" matrix=\"1 1 1\" na=\"1\" name=\""+gname+"\" ne=\"1\" ns=\""+lastStr+"\" rank=\"3\" slices=\"1\" sort=\"yes\" tsize=\"100\" type=\"scan\" >";
     */

    list<RQgroup> groups;

    RQgroup g = RQgroup(gpath);
    g.setAttribute("frames", frame);
    sprintf(str, "%d-%d", 1,last-first+1);
    g.setAttribute("images", string(str));
    sprintf(str, "%d", last-first+1);
    g.setAttribute("ns", string(str));
    g.setAttribute("display", "yes");
    g.addChild(image);
    groups.push_back(g);

    addGroups(groups, "");

    if(last == first) {
      updateStudies();
      notifyVJ("loadFile");
    }
}

string ReviewQueue::getGroupPath(string key) {
    // given image key, make group path
    int p1 = key.find_first_of(" ");
    int p2 = key.find_last_of(" ");
    string dir = key.substr(0, p1);
    string name;
    if (p2 > p1)
        name = key.substr(p1+1, p2-p1-1);
    else
        name = key.substr(p1+1);
    return getGroupPath(dir, name);
}

string ReviewQueue::getGroupPath(string dir, string name) {

    int orient = getPlaneOrient(name);
           
    if (orient == -1)
        return dir;
    else {
        int p1 = dir.find_last_of("/");
        int p2 = dir.find_last_of(".img");
        string gname;
        if (p2 > p1)
            gname = dir.substr(p1+1, p2-p1-4);
        else
            gname = dir.substr(p1+1, dir.length()-p1);
        if (orient == 2)
            gname += "_xy";
        else if (orient == 1)
            gname += "_xz";
        else if (orient == 0)
            gname += "_yz";
        p1 = name.find_last_of("_");
        gname += name.substr(p1, name.length()-p1);
        
        // add contribution from fdf file name to group name, since more than 1 fdf can be in a directory
        gname +="_";
        gname += name.substr(0,name.find(".fdf"));
        
                                            
        return dir+"/"+gname;
        
   
    }
}

int ReviewQueue::getPlaneOrient(string key) {
    // assume if the name of key (dir name 0) contains _xy, _xz, _yz,
    // the image is a plane of 3D data.
    // xy is FRONT_PLANE (2) 
    // xz is TOP_PLANE (1) 
    // yz is SIDE_PLANE (0) 
    // -1 if not a 3D plane

    int p = key.find_first_of(" ", 0);
    if (p != string::npos)
        key = key.substr(p+1);
    if (key.find("_xy") != string::npos) {
        return 2;
    } else if (key.find("_xz") != string::npos) {
        return 1;
    } else if (key.find("_yz") != string::npos) {
        return 0;
    } else {
        return -1;
    }
}

void ReviewQueue::displayPlanes() {
    if (getReal("jviewport", 1) > 2.5) {
        displaySel(DATA_SELECTED_RQ, 1, 1);
    } else {
        //displaySel(DATA_ALL, 1, 1);
        displaySel(DATA_SELECTED_RQ, 1, 1);
    }
}

void ReviewQueue::setReconDisplay(int nimages) {
    int vp = (int)getReal("jviewport", 1);
    if (vp != 3) {
        removeData("all");
    } else deleteData("all");

    if (nimages > 0) {
       double aspect = getAspectRatio();
       GframeManager::get()->splitWindow(nimages, aspect);
    } else
       GframeManager::get()->clearAllFrames();
}

void ReviewQueue::display(int batch) {

   int nimages = m_imageList.size();

   if(nimages < 1 || m_remakeImageList) {
    m_remakeImageList = false;
    int vp = (int)getReal("jviewport", 1);
    if (vp != 3) 
     selectImages(DATA_ALL);
    else
     selectImages(getDisplayMode());
   }
   nimages = m_imageList.size();
   int autolayout = getLayoutMode();
   if(nimages < 1) {
     //Winfoprintf("0 image is loaded or selected");
     if(!autolayout)
       GframeManager::get()->clearAllFrames();
     else 
       GframeManager::get()->deleteAllFrames();
     return;
   }

   int maxImages = (int)getReal("aipMaxImages",128);
   if(nimages < maxImages) maxImages = nimages;
   GframeManager *gfm = GframeManager::get();
   if (autolayout && m_groups.size() == 1) {
        list<RQgroup *>::iterator itr = m_groups.begin();
        int cols = (*itr)->getSlices();
        int rows = (*itr)->getEchoes()*((*itr)->getArray_dim());
        
        int policy = (int)getReal("aipLayoutPolicy", 1);
        // don't layout according to cols and rows only too many images
        if(cols * rows > maxImages) policy = 0;

        if (policy == 1&& nimages == rows*cols && rows > 1&& cols > 1) {
           gfm->splitWindow(cols, rows);
        } else if (policy == 2&& nimages == rows*cols && rows > 1&& cols > 1) {
           gfm->splitWindow(rows, cols);
        } else {
	   double aspect = getAspectRatio();
           gfm->splitWindow(maxImages, aspect);
        }

   } else if (autolayout) {
	double aspect = getAspectRatio();
        gfm->splitWindow(maxImages, aspect);
   } else {
        //gfm->clearAllFrames(); 
	int rows = (int)getReal("aipWindowSplit", 1, 1);
        int cols = (int)getReal("aipWindowSplit", 2, 1);
	gfm->splitWindow(rows,cols);
   }
  
   m_batches = 1;
   int nframe = gfm->getNumberOfFrames();
   if (nframe == 0) {
        // Make some frames to use
	double aspect = getAspectRatio();
        gfm->splitWindow(maxImages, aspect);
        nframe = gfm->getNumberOfFrames();
   }

   if(nframe > 0) m_batches = (int)(0.5+(double)nimages/(double)nframe);
   if(m_batches < 1) m_batches = 1;
   if(nimages > (nframe*m_batches) && (nimages % nframe) != 0) m_batches += 1;

   if(batch < 1) m_batch=1;
   else if(batch > m_batches) m_batch=m_batches;
   else m_batch=batch; 
   setReal("aipBatch",m_batch,true);
   setReal("aipBatches",m_batches, true); 

   grabMouse();
   
    int first =  (m_batch-1)*nframe; 
    int last = first+nframe-1;

    string key;
    DataManager *dm = DataManager::get();
    DataMap *dmap = dm->getDataMap();
    DataMap::iterator pd;
    int f = 0;
    int ind;
    map<int, string>::iterator iitr;
    iitr = m_imageList.begin() ;
    for(iitr = m_imageList.begin(); iitr != m_imageList.end(); ++iitr) {
        if(interuption) return;
        ind = iitr->first;
        key = iitr->second;
        if(ind >= first && ind <= last) {

          //Winfoprintf("dis %d %d %d %d %s",f,ind,first,last,key.c_str());
          if(m_batches==1)
            gfm->setFrameToLoad(ind);
	  else
            gfm->setFrameToLoad(f);
          key = loadKey(key);
          pd = dmap->find(key);
          if (pd != dmap->end()) {
             gfm->loadData(pd->second);
             //gfm->loadData(pd->second, gfm->getFrameByNumber(f-1));
          }
          f++;
        }
    }

    RoiStat::get()->calculate();
    VsInfo::setVsHistogram();

/*
    // calculate VS and redisplay images if VS_UNIFORM.
    int vsMode = VsInfo::getVsMode();
    if (vsMode == VS_UNIFORM) {
        set<string> keys = dm->getKeys(DATA_ALL);
        VsInfo::autoVsGroup(keys);
    }
*/
}

// fill m_groups and m_imageList
void ReviewQueue::selectImages(int displayMode) {

   int dmode = getDisplayMode();
   if(dmode != displayMode) setReal("aipDisplayMode",displayMode,true);

   GframeManager *gfm = GframeManager::get();
   int autolayout = getLayoutMode();
   int globalSort = getGlobalSort();

   m_groups.clear();
   m_selectedImages.clear();

   if(displayMode == DATA_ALL) { // select all images 
      m_imageList.clear();
      if(studies->size()<1) {
        Winfoprintf("0 image is selected or loaded");
	return;
      }
      list<RQstudy>::iterator sitr;
      list<RQnode>::iterator gitr;
      list<RQnode>::iterator iitr;
      list<RQnode> *gl;
      RQgroup *group;
      int ind = 0;
      for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
         gl = sitr->getChildren();
         for (gitr=gl->begin(); gitr != gl->end(); ++gitr) {
            group = (RQgroup *)(&(*gitr));
            group->updateImageList("all");
            m_groups.push_back(group);
         }
      }
      makeImageList_RQ(globalSort);
      list<RQnode *>::iterator itr;
      string key;
      if(m_selectedImages.size() > 0) {
         for (itr = m_selectedImages.begin(); itr != m_selectedImages.end(); ++itr) {
	    key = (*itr)->getKey();
            m_imageList.insert(map<int, string>::value_type(ind,key));
	    loadKey(key, (*itr)->getAttribute("gindex"));
	    ind++;
         }
      } else {
	 Winfoprintf("0 image is selected.");
      }
   } else if(displayMode == DATA_GROUP) { // select all images of the group   
      RQnode *node = getNode(getActiveKey());
      if (node == (RQnode *)NULL) {
	Winfoprintf("0 image is selected");
        return;
      }
      RQgroup *group = (RQgroup *)node->getParent();
      if (group == (RQgroup *)NULL) {
	Winfoprintf("No group defined for image %s",node->getPath().c_str());
        return;
      }

      group->updateImageList("all");
      m_imageList.clear();
      m_groups.push_back(group);
      ChildList *il = group->getChildren();
      list<RQnode>::iterator iitr;
      int ind = 0;
      string key;
      for (iitr = il->begin(); iitr != il->end(); ++iitr) {
	  key = iitr->getKey();
          m_imageList.insert(map<int, string>::value_type(ind,key));
	  loadKey(key, iitr->getAttribute("gindex"));
          ind++;
      }
   } else if(displayMode == DATA_SELECTED_RQ) {
      if(studies->size()<1) return;
      list<RQstudy>::iterator sitr;
      list<RQnode>::iterator gitr;
      list<RQnode> *gl;
      RQgroup *group;
      map<int, RQgroup *> gmap;
      int count = 0;
      int fnum;
      // order groups according to "frames" attribute
      for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
         gl = sitr->getChildren();
         for (gitr=gl->begin(); gitr != gl->end(); ++gitr) {
            group = (RQgroup *)(&(*gitr));
            if(group->getAttribute("display") != "yes") continue;
            group->updateImageList(group->getAttribute("images"));
            string fsel = group->getAttribute("frames");
	    if(fsel.find("-") != string::npos) 
               fnum = atoi(fsel.substr(0,fsel.find("-")).c_str());
	    else  
	       fnum = atoi(fsel.c_str()); 
            if(gmap.find(fnum) != gmap.end()) { // key already exists
	       count++;
               break;
	    }
            gmap.insert(map<int, RQgroup *>::value_type(fnum,group));
	 }
      }
      if(count>0) { // frames overlap. order gmap according to order of nodes
        gmap.clear();
        fnum = 0;
        for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
          gl = sitr->getChildren();
          for (gitr=gl->begin(); gitr != gl->end(); ++gitr) {
            group = (RQgroup *)(&(*gitr));
            if(group->getAttribute("display") != "yes") continue;
            group->updateImageList(group->getAttribute("images"));
	    fnum++;
            gmap.insert(map<int, RQgroup *>::value_type(fnum,group));
          }
        }
      }

      // fill groups according gmap (ordered groups)
      map<int, RQgroup *>::iterator mitr;
      char frames[64];
      count = 0;
      for(mitr = gmap.begin(); mitr != gmap.end(); ++mitr) { 
        sprintf(frames,"%d-",count+1); 
        mitr->second->setAttribute("frames",frames);
	m_groups.push_back(mitr->second);
        count += mitr->second->getSelSize();
      }
      notifyVJ("frames");
      
      m_imageList.clear();
      string key;
      if(m_groups.size()>0) {
        makeImageList_RQ(globalSort);
        list<RQnode *>::iterator itr;
        int ind = 0;
        if(m_selectedImages.size() > 0) {
         for (itr = m_selectedImages.begin(); itr != m_selectedImages.end(); ++itr) {
	    key = (*itr)->getKey();
            m_imageList.insert(map<int, string>::value_type(ind,key));
	    loadKey(key, (*itr)->getAttribute("gindex"));
	    ind++;
         }
        } else {
	 Winfoprintf("0 image is selected.");
        }
      } else {
        int vp = (int)getReal("jviewport", 1);
        if (vp != 3) {  
          selectImages(DATA_ALL);
	  return;
        } 
      }
   } else if(displayMode == DATA_SELECTED_FRAMES) { 
      list<string> keylist;
      list<string>::iterator sitr;
      if (WinMovie::get()->getGuestFrame() == -1&& WinMovie::get()->getOwnerFrame() == -1)
            keylist = GframeManager::get()->getSelectedKeylist();
      if (keylist.size() > 0) {
            m_imageList.clear();
            int ind = 0;
            for (sitr = keylist.begin(); sitr != keylist.end(); ++sitr) {
                m_imageList.insert(map<int, string>::value_type(ind,(*sitr)));
                ind++;
            }
      } else {
	 Winfoprintf("0 image is selected.");
      }
   } else if(displayMode == DATA_SELECTED_IMAGES) {
      m_imageList.clear();
      makeSelections(getGlobalSort(), autolayout, getSelection());
      list<RQnode *>::iterator itr;
      int ind = 0;
      string key;
      if(m_selectedImages.size() > 0) {
         for (itr = m_selectedImages.begin(); itr != m_selectedImages.end(); ++itr) {
	    key = (*itr)->getKey(); 
            m_imageList.insert(map<int, string>::value_type(ind,key));
	    loadKey(key, (*itr)->getAttribute("gindex"));
	    ind++;
         }
      } else if(m_groups.size() > 0) {
         makeImageList_RQ(globalSort);
         if(m_selectedImages.size() > 0) {
            for (itr = m_selectedImages.begin(); itr != m_selectedImages.end(); ++itr) {
	       key = (*itr)->getKey(); 
               m_imageList.insert(map<int, string>::value_type(ind,key));
	       loadKey(key, (*itr)->getAttribute("gindex"));
	       ind++;
            }
         }
      } else {
	 Winfoprintf("0 image is selected.");
      }
   } else if(displayMode == DATA_DISPLAYED) { // select display batch
      m_imageList.clear();
      spGframe_t gf;
      GframeList::iterator gfi;
      spImgInfo_t img;
      int ind = 0;
      for (gf=gfm->getFirstFrame(gfi); gf != nullFrame; gf
            =gfm->getNextFrame(gfi)) {
        if(gf == nullFrame) continue;
        
        img = gf->getFirstImage();
        if(img == nullImg) continue;
   
        m_imageList.insert(map<int, string>::value_type(ind,img->getDataInfo()->getKey()));
	ind++;
      }
      
   } else if(displayMode == DATA_SELECTED) { // no change for m_imageList
      if(m_imageList.size() < 1) {
        Winfoprintf("0 image is selected");
 	return;
      }
   }

    DataManager *dm = DataManager::get();
    dm->makeShortNames();
    dm->numberData();
}

void ReviewQueue::displayRQ() {
    map<int, string> images; // int is frame id, string is image key
    map<int, string>::iterator mitr;
    int framemax = 0;

    RQparser *ps = RQparser::get();
    list<int> sl;

    list<RQstudy>::iterator sitr;
    list<RQnode>::iterator gitr;
    list<RQnode>::iterator iitr;
    list<RQnode> *gl;
    list<RQnode *> *l;
    RQgroup *group;
    // make images hash table (map). 
    for (sitr = studies->begin(); sitr != studies->end(); ++sitr) {
         gl = sitr->getChildren();
         for (gitr=gl->begin(); gitr != gl->end(); ++gitr) {
            group = (RQgroup *)(&(*gitr));
            if(group->getAttribute("display") != "yes") continue;
            group->updateImageList(group->getAttribute("images"));
            l = group->getImageList();
            string fsel = group->getAttribute("frames");
            sl = ps->parseFrames(fsel,1,1,l->size()); // augument 1,1 are not used

	    // sl is a list of integers of frame numbers for the group
	    list<int>::iterator slItr; 
      	    list<RQnode *>::iterator itr;
	    for (slItr = sl.begin(), itr=l->begin(); slItr != sl.end() && itr != l->end(); ++slItr,++itr) {
		int frame = (*slItr);
		string key = (*itr)->getKey();
		images.insert(map<int, string>::value_type(frame,key));
		if(frame>framemax) framemax=frame;
            }
	 }
    }

    GframeManager *gfm = GframeManager::get();
    DataManager *dm = DataManager::get();
    DataMap *dmap = dm->getDataMap();
    DataMap::iterator pd;
    if(gfm->getNumberOfFrames() < framemax) {
        double aspect = getAspectRatio();
        gfm->splitWindow(framemax, aspect);
        if(gfm->getNumberOfFrames() < framemax) {
	   Winfoprintf("Frame number too big.");
	   return;
	}
    } else gfm->clearAllFrames(); 
    if(getLayoutMode()>0) setReal("aipAutoLayout",0,true); 
   
    m_batch=m_batches=1;
    setReal("aipBatch",m_batch,true);
    setReal("aipBatches",m_batches, true); 
    for(mitr=images.begin(); mitr!=images.end(); ++mitr) {
//Winfoprintf("map %d %d %s",framemax,mitr->first,mitr->second.c_str());
          gfm->setFrameToLoad(mitr->first-1);
          string key = loadKey(mitr->second);
          pd = dmap->find(key);
          if (pd != dmap->end()) {
             gfm->loadData(pd->second);
          }
    }
}
