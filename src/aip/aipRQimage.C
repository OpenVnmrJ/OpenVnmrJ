/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "aipRQimage.h"
#include "aipRQgroup.h"
#include "aipReviewQueue.h"
#include "aipDataManager.h"
#include "aipDataInfo.h"
#include "aipVolData.h"
#include "aipVnmrFuncs.h"

extern "C" {
void set_batch_function(int on);
}

RQimage::RQimage() :
    RQnode() {
    setParent((RQnode *)NULL);
    initImage();
}

RQimage::RQimage(RQnode * parent, string str) :
    RQnode() {
    setParent(parent);
    if (str.find("filenode", 0) != string::npos) { // str is xml
        readNode(str);
    } else if (str.find("/", 0) == 0) { // str is key
        initImage(str);
    } else {
        initImage();
    }

}

RQimage::RQimage(string str) :
    RQnode() {
    setParent((RQnode *)NULL);
    if (str.find("filenode", 0) != string::npos) { // str is xml 
        readNode(str);
    } else if (str.find("/", 0) == 0) { // str is key
        initImage(str);
    } else {
        initImage();
    }
}

void RQimage::initImage(string str) {
    int rank = 2;
    string dir = "";
    string name = "";
    string ext = "";
    string copy = " 0";
    int slice = 1;
    int array_index = 1;
    int echo = 1;
    set_batch_function(1);  // turn on grahpics batch mode
    if (str.find("/", 0) == 0) { // str is key or path.

        int i1;
        if ((i1 = str.find_last_of(" ")) != string::npos) {
            copy = str.substr(i1);
            str = str.substr(0, i1);
        } else
            copy = ReviewQueue::get()->getNextCopy(IMAGE, str);

        if ((i1 = str.find(" ", 1)) == string::npos)
            str.replace(str.find_last_of("/"), 1, " ");

        if ((i1 = str.find(" ", 1)) != string::npos) {
            dir = str.substr(0, i1);
            name = str.substr(i1+1);
        }

        getIndex4Key(str + copy, &rank, &slice, &array_index, &echo);

        if (ReviewQueue::get()->getPlaneOrient(name) == -1) {
            if ((i1 = name.find(".")) != string::npos) {
                ext = name.substr(i1, name.length()-i1);
                name = name.substr(0, i1);
            }
        } else {
            i1 = name.find(".fdf", 0);
            i1 = name.find("_", i1+4);
            i1 = name.find("_", i1+1);
            slice = 1 + atoi(name.substr(i1+1,name.length()- name.find("_",i1+1)).c_str());
        }
    }

    setAttribute("type", "img");
    setAttribute("dir", dir);
    setAttribute("name", name);
    setAttribute("ext", ext);
    setAttribute("copy", copy);
    setAttribute("ns", slice);
    setAttribute("na", array_index);
    setAttribute("ne", echo);
    setAttribute("frame", "1");
    setAttribute("batch", "1");
    setAttribute("nid", "");
    setAttribute("shortName", "");
    setAttribute("lindex", "");
    setAttribute("gindex", "");
    setAttribute("rank", rank);

}

void RQimage::getIndex4Key(string key, int *rank, int *slice, int *image,
        int *echo) {
    int i1, i2;

    if ((i1 = key.find(" ", 1)) == string::npos)
        return;
    if ((i2 = key.find(".fdf", i1+1)) == string::npos)
        return;

    string fname = key.substr(i1+1, i2-i1-1);

    int s = fname.find("slice", 0);
    int i = fname.find("image", 0);
    int e = fname.find("echo", 0);
    int c = fname.find("coil", 0);
    int l = fname.length();

    if (c == string::npos && s == 0&& i == 8&& e == 16) {

        *rank = 2;
        // fname is slicexxximagexxxechxxx

        *slice = atoi(fname.substr(s+5,i-s-5).c_str());
        *image = atoi(fname.substr(i+5,e-i-5).c_str());
        *echo = atoi(fname.substr(e+4,l-e-4).c_str());
/*
    } else if (c == string::npos && i == 0) {

        *rank = 2;
        // fname is imagexxxx

        int slices=ind, array_dim=1, echoes=1;
        string path = key.substr(0, key.find(" ", 1));
        RQgroup::getDimFromProcpar(path, &slices, &array_dim, &echoes);

        int ind = atoi(fname.substr(i+5,4).c_str());
        int n = 0;
        for (int i=0; i<echoes; i++) {
            for (int j=0; j<array_dim; j++) {
                for (int k=0; k<slices; k++) {
                    n++;
                    if (n == ind) {
                        *echo = i+1;
                        *image = j+1;
                        *slice = k+1;
                        break;
                    }
                }
            }
        }
*/
    } else {
        spDataInfo_t dataInfo = DataManager::get()->getDataInfoByKey(key, true);
        if (dataInfo == (spDataInfo_t)NULL) {
            key = ReviewQueue::get()->loadKey(key);
            dataInfo = DataManager::get()->getDataInfoByKey(key);
        }
        // if 3D data, won't find in DataMap.
        if (dataInfo != (spDataInfo_t)NULL) {
            *rank = 2;  // not so sure about that
            if(!dataInfo->st->GetValue("rank", *rank))
                *rank=2;
            if (!dataInfo->st->GetValue("slice_no", *slice))
                *slice = 1;
            if (!dataInfo->st->GetValue("array_index", *image))
                *image = 1;
            if (!dataInfo->st->GetValue("echo_no", *echo))
                *echo = 1;
            // if coil parameter exists, use it in place of image,
            // echo, or slice, if one of the size is 1.
            int nc = 1;
            dataInfo->st->GetValue("coils", nc);
            if (nc > 1) {
                RQnode *p = getParent();
                int n = 1;
                dataInfo->st->GetValue("array_dim", n);
                //if(n == 1 || n == nc) {
                if (n == 1) {
                    if (!dataInfo->st->GetValue("coil", *image))
                        *image = 1;
                    if (n != nc && p != (RQnode *)NULL)
                        p->setAttribute("na", nc);
                } else {
                    dataInfo->st->GetValue("echoes", n);
                    if (n == 1) {
                        if (!dataInfo->st->GetValue("coil", *echo))
                            *echo = 1;
                        if (p != (RQnode *)NULL)
                            p->setAttribute("ne", nc);
                    } else {
                        dataInfo->st->GetValue("slices", n);
                        if (n == 1) {
                            if (!dataInfo->st->GetValue("coil", *slice))
                                *slice = 1;
                            if (p != (RQnode *)NULL)
                                p->setAttribute("ns", nc);
                        }
                    }
                }
            }
        } else {
            *rank = 2;
            *slice = 1;
            *image = 1;
            *echo = 1;
            if (key != "")
                dataInfo = VolData::get()->dataInfo;
            if (dataInfo != (spDataInfo_t)NULL) {
                DDLSymbolTable *st = dataInfo->st;
                if (st) {
                    st->GetValue("rank", *rank);
                    /*
                     if(*rank == 3) {
                     // this will not be added to DataMap or RQ. 
                     // instead, 3 groups will be made for a 3D data.
                     int xyLast = 1;
                     int xzLast = 1;
                     int yzLast = 1;
                     st->GetValue("matrix", xyLast, 2);
                     st->GetValue("matrix", xzLast, 1);
                     st->GetValue("matrix", yzLast, 0);
                     VolData::get()->extract_planes(FRONT_PLANE, 0, xyLast-1, 1);
                     VolData::get()->extract_planes(TOP_PLANE, 0, xzLast-1, 1);
                     VolData::get()->extract_planes(SIDE_PLANE, 0, yzLast-1, 1);
                     }
                     */
                }
            }
        }
    }
}

string RQimage::getIndexStr() {
    return getAttribute("ns") +" "+ getAttribute("na") +" "+ getAttribute("ne");
}

string RQimage::getStudyPath() {
    string path = getAttribute("dir");
    path = path.substr(0, path.find_last_of("/"));

    int pos = path.find("/data", path.length()-5);
    if (pos != string::npos)
        path = path.substr(0, path.length()-5);

    return path;
}

string RQimage::getGroupPath() {
    return ReviewQueue::get()->getGroupPath(getAttribute("dir"), getAttribute("name"));
}
