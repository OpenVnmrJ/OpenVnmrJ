/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <math.h>
#include <iostream>
#include <list>
#include <set>
#include <map>
#include <string>
#include <algorithm>
#include <fcntl.h>
using std::min;
#ifdef __INTERIX
#define ROUND(x)   floor((x)+0.5)
#else
#define ROUND(x)   round(x)
#endif
using namespace std;

#include "aipStderr.h"
#include "aipVnmrFuncs.h"
//#include "ddlSymbol.h"
#include "aipCommands.h"
#include "aipDataInfo.h"
#include "aipCFuncs.h"
#include "aipGframe.h"
#include "aipGframeManager.h"
#include "aipDataManager.h"
#include "aipImgInfo.h"
#include "aipMouse.h"
#include "aipRoiManager.h"
#include "aipVnmrFuncs.h"
#include "aipVolData.h"
#include "aipOrthoSlices.h"
#include "aipWinMovie.h"
#include "aipMovie.h"
#include "aipWinRotation.h"
#include "group.h"
#include "aipWinMath.h"
#include "aipReviewQueue.h"
#include "aipVsInfo.h"
#include "vnmrsys.h"

using namespace aip;


#define DEG_TO_RAD (M_PI/180.)
#define D360 360.
#define D180 180.
#define D90 90.
#define D270 270.

std::set<string> DataManager::selectedSet;

#ifdef TEST
/***********  TEST  **************/
extern bool memTest();
/***********  END TEST  **************/
#endif


DataManager *DataManager::dataManager = NULL;

extern "C" {
    void aip_movie_image(int on);
    int tensor2euler(float* theta, float* psi, float* phi, float* orientation);
    void snapAngle(float* a);
}

/*
 * Receive new image data.
 */
/* C-INTERFACE */
void
aipInsertData(dataStruct_t *data)
{
    spDataInfo_t dataInfo = spDataInfo_t(new DataInfo(data, NULL));
    if(dataInfo == (spDataInfo_t)NULL) {
	STDERR("aipDataManager: DataInfo returned NULL pointer");
	return;
    }

    string key = dataInfo->getKey();
    DataManager *dm = DataManager::get();
    dm->insert(dataInfo);
    if (isDebugBit(DEBUGBIT_7)) {
        fprintf(stderr,"Inserted: key=%s, data[32640]=%g, %s\n",
                key.c_str(), data->data[32640], data->type);
    }
}

void
DataManager::insert(spDataInfo_t dataInfo)
{
    //data ia loaded as needed. don't want it to reset movieIter. 
    // only need to notify WinMovie when data ia deleted.
    //WinMovie::dataMapChanged();

    string key = dataInfo->getKey();
    dataMap->erase(key);	// Delete any old version.
    dataMap->insert(DataMap::value_type(key, dataInfo));
    //numberData();
}

int
DataManager::getNumberOfImages()
{
    return dataMap->size();
}

void
DataManager::numberData()
{
    DataMap::iterator pd;
    int i;
    for (i = 1, pd = dataMap->begin(); pd != dataMap->end(); ++pd, ++i) {
        pd->second->setDataNumber(i);
    }
}

spDataInfo_t
DataManager::getDataByNumber(int imgNumber)
{
    DataMap::iterator pd;
    int i;
    for (i=0, pd=dataMap->begin(); pd != dataMap->end(); ++pd, ++i) {
        if (i == imgNumber) {
            return pd->second;
        }
    }
    return (spDataInfo_t)NULL;
}

#ifdef TEST
/***********  TEST  **************/
bool
DataManager::dataCheck()
{
    DataMap::iterator pd;
    int i;
    for (i=0, pd=dataMap->begin(); pd != dataMap->end(); ++pd, ++i) {
        long n = *pd->second.pn;
        //fprintf(stderr,"dataCheck: i=%d, n=%d\n", i, n);
        if (n < 0 || n > 10) {
            fprintf(stderr,"dataCheck FAILED: i=%d, n=%d\n", i, n);
            return false;
        }
    }
    fprintf(stderr,"dataCheck: nullFrame.px=0x%x, nullFrame.n=%d\n",
            nullFrame.px, *nullFrame.pn);
    return true;
}
/***********  END TEST  **************/
#endif
 
double
DataManager::getAspectRatio()
{
    double aspect = 1;
    if (getNumberOfImages() > 0) {
	double spans[3];
	DataMap::iterator pd = dataMap->begin();
	pd->second->getSpatialSpan(spans);
	if (spans[0] * spans[1] != 0) {
	    aspect = spans[0] / spans[1];
	}
        int rotation = WinRotation::calcRotation(pd->second);
        if (rotation & 4) {
            aspect = 1 / aspect; // Image gets transposed for display
        }
    }
    return aspect;
}

//DataInfo *
//aipGetData(string key)
//{
//    return (*dataMap)[key];
//}

DataManager::DataManager()
{
    dataMap = new DataMap;
}

/*
 * Returns the one DataManager instance.  Creates one if it has
 * not been created yet.  Updates window size.
 */
/* PUBLIC STATIC */
DataManager *DataManager::get()
{
    if (!dataManager) {
	dataManager = new DataManager();
    }
    return dataManager;
}

/*
 * Load an FDF file
 */
/* STATIC VNMRCOMMAND */
int
DataManager::aipLoadFile(int argc, char *argv[], int retc, char *retv[])
{
    ReviewQueue *rq = ReviewQueue::get();
    //DataManager *dm = DataManager::get();
    bool ok = false;
    if (argc == 3) {
	ok = rq->loadFile(argv[1], "", atoi(argv[2]), true) != 0;
	//ok = dm->displayFile(argv[1], atoi(argv[2])) != "";
    } else if (argc == 2) {
	ok = rq->loadFile(argv[1]) != 0;
	//ok = dm->loadFile(argv[1]) != "";
    } else {
	return proc_error;
    }

    if (ok) {
        VsInfo::setVsHistogram();
        return proc_complete;
    } else {
	return proc_error;
    }
}

/*
 * Delete data
 */
/* STATIC VNMRCOMMAND */
int
DataManager::aipDeleteData(int argc, char *argv[], int retc, char *retv[])
{
  if(VolData::get()->showingObliquePlanesPanel()) { 
      VolData::get()->showObliquePlanesPanel(false);
      OrthoSlices::get()->setShowCursors(false);
  }

  string str = "all";
  if(argc > 1) str = argv[1];
  if(getReal("jviewport", 1) < 3) {
     ReviewQueue::get()->removeData(str); // delete data and clear RQ
  } else {
     ReviewQueue::get()->deleteData(str); // delete data but not clear RQ
  }
/*
  if(getReal("jviewport", 1) < 3) {
    string str = "all";
    if(argc > 1) str = argv[1];

    if(!VolData::get()->showingObliquePlanesPanel())
        ReviewQueue::get()->removeData(str);
    return proc_complete;
  } else {
    bool selected = false;
    string key = "";

    argv++; argc--;
    for ( ; argc > 0; argv++, argc--) {
	if (strncasecmp(*argv, "sel", 3) == 0) {
	    selected = true;
	}
	if (*argv[0] == '/') {
	    key = *argv;
	}
    }

    struct stat fstat;
    DataManager *dm = DataManager::get();
    if (selected) {
	dm->deleteSelectedData();

    } else if (key != "" && strstr(key.c_str(), " ")) {
	dm->deleteDataByKey(key);
    } else if (key != "" && stat(key.c_str(), &fstat) == 0) { 
	if(S_ISREG(fstat.st_mode) && !strstr(key.c_str(),".fdf")) {
	   // if a file but not ends with .fdf, it is a file contains the list.
	   dm->deleteDataInList(key.c_str());
        } else{
	   // will recursively delete data corresponds to all .fdf files 
	   // in the dir. the dir can be a fdf file.
	   dm->deleteDataInDir(key.c_str());
	}
    } else {
	dm->deleteAllData();
    }

  }
*/
    return proc_complete;
}

void
DataManager::deleteDataInList(const char *path)
{
    char  buf[MAXSTR];
    FILE *fp = fopen(path, "r");
     
    if(!fp) return;
    DataManager *dm = DataManager::get();
    
    string key = "";
    while (fgets(buf,sizeof(buf),fp)) {
        if(strlen(buf) > 1 && buf[0] != '#') {
	    if(strstr(buf,"\n")) strcpy(strstr(buf,"\n"), ""); 
	    key = buf;
	    key = key.replace(key.find_last_of("/"), 1," ");
	    dm->deleteDataByKey(key);
        }
    }

    fclose(fp);
    //unlink(path);
}

void
DataManager::deleteDataInDir(const char *path)
{
    DIR *dirp;
    struct dirent *dp;
    const int BUFLEN = 1024;
    char str[BUFLEN];
    struct stat fstat;

    if(stat(path, &fstat) != 0) {
	sprintf(str, "%s: \"%.512s\"", strerror(errno), path);
	ib_errmsg(str);
	return;
    }

    string key = path;
    if(S_ISREG(fstat.st_mode) && strstr(path, ".fdf")) {
	key = key.replace(key.find_last_of("/"), 1," ");
        set<string> keys;
        DataMap::iterator pd;
        for (pd = dataMap->begin(); pd != dataMap->end(); ++pd) {
	  if(pd->second->getNameKey().find(key) == 0) {
            keys.insert(pd->second->getKey());
	  }
        }

        set<string>::iterator sitr;
        for (sitr = keys.begin(); sitr != keys.end(); ++sitr) {
	    deleteDataByKey(*sitr);
 	}

	return;
    } else if (S_ISDIR(fstat.st_mode)) {

    // Open the directory
    dirp = opendir(path);
    if(dirp == NULL) {
	sprintf(str, "%s: \"%.512s\"", strerror(errno), path);
	ib_errmsg(str);
	return;
    }
    // Loop through the file list looking for fdf files.
    while ((dp = readdir(dirp)) != NULL && !interrupt()) {
	if (*dp->d_name != '.') {
	    sprintf(str, "%s/%s",path,dp->d_name);
	    deleteDataInDir(str);	
	} 
    }
    closedir(dirp);

    }
}

void
DataManager::deleteAllData()
{
    // TODO: When a frame is deleted, go through the selected and active
    // ROI lists and delete all those whose pOwnerFrame is the frame
    // deleted.
    RoiManager::get()->deleteSelectedRois(); // Should not really be necessary
    GframeManager* gfm = GframeManager::get();
    gfm->deleteAllFrames(aipHasScreen());
    gfm->clearFrameCache();
    
    dataMap->clear();
    WinMovie::dataMapChanged();

    RoiStat::get()->calculate();
    VsInfo::setVsHistogram();
}

void
DataManager::deleteSelectedData()
{

    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;
    GframeList::iterator gfi;
    for (gf=gfm->getFirstSelectedFrame(gfi);
	 gf != nullFrame;
	 gf=gfm->getNextSelectedFrame(gfi))
    {
	spViewInfo_t vi;
	if ((vi=gf->getFirstView()) != nullView) {
	    string key = vi->imgInfo->getDataInfo()->getKey();
            if (isDebugBit(DEBUGBIT_7)) {
                fprintf(stderr,"deleteSelectedData(): key=%s\n",
                        key.c_str());
            }
            DataMap::iterator pd = dataMap->find(key);
            if (pd != dataMap->end()) {
                dataMap->erase(key); // Delete data from our list
                WinMovie::dataMapChanged();
            }

            gfm->unsaveFrame(key);
            gf->clearFrame();
            if (aipHasScreen()) {
                gf->draw();
            }
	}
    }
    RoiStat::get()->calculate();
    VsInfo::setVsHistogram();
    //makeShortNames();
    //numberData();
}



string DataManager::javaFile(const char *path, 
		int *nx, int *ny, int *ns,
		float *sx, float *sy, float *sz, 
		float **jdata) {
	float *tdata=NULL;
	char fname[1024];
	char msg[1024];
	if (interrupt()) {
		return "";
	}

	(void)strcpy(fname, path);

	if (access(fname, R_OK) != 0) {
		// File not accessible
		sprintf(msg, "Cannot read file: %.1000s", path);
		ib_errmsg(msg);
		return "";
	}

	DDLSymbolTable *st = ParseDDLFile(fname);// Get header
	if (!st)
		return "";

	// get matrix dimensions
	int w, h, slices=1;
	int rank=1;
	st->GetValue("matrix", w, 0);
	st->GetValue("matrix", h, 1);

	st->GetValue("rank", rank);
	if (rank > 2) {
		VolData *vdat=VolData::get();
		st->GetValue("matrix", slices, 2);
		ReviewQueue::get()->removeData("all");
		int tlen; char *tstr;
		tlen = strlen(fname) + 50;
		tstr = (char *)malloc(tlen*(sizeof(char)));
		//sprintf(tstr, "aipShow('%s','','dnd',1)\n", fname);
		sprintf(tstr, "aipShow('%s','','',1)\n", fname);
		execString(tstr);
		(void)free(tstr);
		if(vdat->dataInfo!=(spDataInfo_t)NULL)
			tdata=vdat->dataInfo->getData();
	}
	else{ 
		// note: since javaFile is currently called only for 3D fdf files we should
		//       never end up here. (but just in case ..)
		string key=loadFile(path,false);
		spDataInfo_t dataInfo = getDataInfoByKey(key);
		if(dataInfo!=(spDataInfo_t)NULL)
			tdata=dataInfo->getData();
	}
	if(tdata==NULL){
		sprintf(msg, "error reading: %.1000s", path);
		ib_errmsg(msg);
		return "null data";
	}
	
	double spanx,spany,spanz;
	st->GetValue("span", spanx, 0);
	st->GetValue("span", spany, 1);
	st->GetValue("span", spanz, 2);
	
	*sx=(float)spanx;
	*sy=(float)spany;
	*sz=(float)spanz;
	
	*nx=w;
	*ny=h;
	*ns=slices;
	*jdata=tdata;

	return ("");
}


/*
 * Load an FDF file
 * Returns the key in the 
 */
string DataManager::loadFile(const char *name, bool mcopy) {
	//fprintf(stderr,"loadFile(%s)\n", name);
	char *str;
	char path[1024];
	char auxpath[1024];
	char msg[1024];
	int rank=0;
	if (interrupt()) {
		return "";
	}

	strcpy(path, name);

	// copies is originally the number of times the same file is loaded.
	// by default, it is set to the number of times the same file is loaded.
	// it may also be specified by attach it to the path with a space. 
	int copies;
	if (strstr(path, " ")) {
		copies = atoi(strstr(path, " "));
		strcpy(strstr(path, " "), "");
	} else if (mcopy) {
		copies = findCopiesInDataMap(path) + 1;
	} else {
		copies = findCopiesInDataMap(path);
	}

	if (access(path, R_OK) != 0) {
		// File not accessible
		sprintf(msg, "Cannot read file: %.1000s", path);
		ib_errmsg(msg);
		return "";
	}

	DDLSymbolTable *st = ParseDDLFile(path);// Get header
	if (!st)
		return "";

        
	if (!st->GetValue("spatial_rank", str) || !strstr(str,"fov")) 
            return "";

	st->SetCopyNumber(copies);

	// Get any auxiliary header info
	sprintf(auxpath, "%.1019s.aux", path);
	DDLSymbolTable *st2= NULL;
	if (access(auxpath, R_OK) == 0) {
		st2 = ParseDDLFile(auxpath);
		/* {
		 char *pc = (char *)"none";
		 st2->GetValue("vsFunction", pc);
		 fprintf(stderr,"Got aux header: function=%s\n", pc);
		 st2->PrintSymbols(cerr);
		 }  */
	} else {
		st2 = new DDLSymbolTable();
		if (st2 != NULL)
			st2->SetSrcid(auxpath);
	}

	// TEST:
	//st->Delete();
	//st = NULL;
	//st = ParseDDLFile(path);// Get header
	// END TEST
	
	if (!st) {
		return "";
	}
	st->SetValue("filename", path);
	// Remove any cached version of this data
	GframeManager::get()->unsaveFrame(DataInfo::getKey(st));

	VolData *vdat = VolData::get();

	if (!st->GetValue("spatial_rank", str) &&!st->GetValue("subrank", str)) 
		rank=0;
	else if((strcmp(str, "3dfov") == 0))
		rank=3;
	else if((strcmp(str, "2dfov") == 0))
	    rank=2;		
	
	
	char *mallocError;
	if(rank==3 && !vdat->getOverlayFlg()){
		vdat->setDfltMapFile();
		vdat->setMatrixLimits();
	    mallocError=vdat->setVolData(st);
	}
	else
		mallocError=st->MallocData();
	
	if (mallocError[0] != 0) {
		// Got an error message
		ib_errmsg(mallocError);
		delete st;
		return "";
	}
	/* debug byteswap
	 st->SaveSymbolsAndData("/tmp/junk.data");
	 */
	// Remove any cached version of this data
	// GframeManager::get()->unsaveFrame(DataInfo::getKey(st));

	// We need to determine if this is a 3D file so we can enable/disable
	// the Extract Panel Tab
	if (rank==0) {
		fprintf(stderr,"No \"spatial_rank\" in FDF header\n");
	} else {
		if (rank==3 && !vdat->getOverlayFlg()) {
			// 3D, enable the extract panel
			vdat->enableExtractSlicesPanel(true);
			vdat->showObliquePlanesPanel(true);

			// Set the filepath for this image file
			vdat->setVolImagePath(path);
		} else if(rank != 3) {
			// not 3D, disable the extract panel
			vdat->enableExtractSlicesPanel(false);
			vdat->showObliquePlanesPanel(false);
		}
	}
	return loadFile(path, st, st2);
}

/*
 * Load an FDF file that has already been read from disk.
 * Input is DDL Symbol Table with valid data pointer.
 */
string DataManager::loadFile(const char *inpath, DDLSymbolTable *st,
		DDLSymbolTable *st2, char *orientStr) {
	string key = "";

	if (inpath == NULL) {
		inpath = st->GetSrcid();
	}
	dataStruct_t *ds = new dataStruct_t;

	// default orientStr is null. But extracted planes will be "xy","xz",or,"yz"
        if(orientStr != NULL) strcpy(ds->planeOrient,orientStr);
        else strcpy(ds->planeOrient,"");

	if (DataInfo::setDataStructureFromSymbolTable(ds, st)) {
		spDataInfo_t dataInfo = spDataInfo_t(new DataInfo(ds, st, st2));
		if (dataInfo == (spDataInfo_t)NULL) {
			delete ds;
			delete st;
			delete st2;
			STDERR("aipDataManager: DataInfo returned NULL pointer");
			return "";
		}
		//dataInfo->st = st;
		if (ds->rank == 3) {
		    // Save only one 3D data set
		    VolData *vdat = VolData::get();
			
		    if(!vdat->getOverlayFlg()) {
			if(vdat->dataInfo != (spDataInfo_t)NULL){
				key = vdat->dataInfo->getKey();
			    deleteDataByKey(key);
			}
			vdat->dataInfo = dataInfo;
		    }
		    insert(dataInfo);
		} else {
			insert(dataInfo);
			//numberData();
		}
		//makeShortNames();
		key = dataInfo->getKey();
	} else {
		delete ds;
		delete st;
		delete st2;
	}
	return key;
}

int
DataManager::findCopiesInDataMap(const char *path)
{
    string key = path;
    key = key.replace(key.find_last_of("/"), 1," ");
    string currentKey = "";

    DataMap::iterator pd;
    for (pd = dataMap->begin(); pd != dataMap->end(); ++pd) {
	if(pd->second->getNameKey().find(key) == 0 &&
		pd->second->getNameKey() > currentKey) {
	    currentKey = pd->second->getNameKey();
	}
    }

    if(currentKey != "") {
       key = currentKey.substr(currentKey.find_last_of(" "));
       int copies = atoi(key.c_str());
       return copies;
    } else {
       return 0;
    }
}

/*
 * Load all FDF files in the given directory.
 * Usage: aipLoadDir dir_path
 */
/* STATIC VNMRCOMMAND */
int
DataManager::aipLoadDir(int argc, char *argv[], int retc, char *retv[])
{
    if(argc > 2) ReviewQueue::get()->loadData(argv[1], "", atoi(argv[2]), true);
    else ReviewQueue::get()->loadData(argv[1]);

    RETURN;

/*
    const int BUFLEN = 1024;
    struct dirent *dp;
    DIR *dirp;
    const char *filename;
    const char *found;
    char str[BUFLEN + 80];
    char arg1[BUFLEN + 4];
    struct stat fstat;
    int frameNumber = 0;
    bool mcopy = false;

    DataManager *dm = DataManager::get();

    if (argc < 2) {
	fprintf(stderr, "Usage: %s path [frame#]\n", argv[0]);
	return proc_error;
    }
    argc--;
    argv++;

    strncpy(arg1, argv[0], BUFLEN - 1);
    arg1[BUFLEN - 1] = '\0';
    argc--;
    argv++;

    if (argc >= 1 && strcmp(argv[0], "copies") == 0) {
	mcopy = true;
	argc--;
	argv++;
    }
    if (argc >= 1) {
        frameNumber = atoi(argv[0]);
    }

    // Do we have a file or a directory?
    if (stat(arg1, &fstat) != 0) {
        // Not found, append ".fdf"
        strcat(arg1, ".fdf");
        if (stat(arg1, &fstat) != 0) {
            // Not found, report original file name
            sprintf(str, "%s: \"%.1024s\"", strerror(errno), arg1);
            ib_errmsg(str);
            return proc_error;
        }
    }
    if (S_ISREG(fstat.st_mode)) {
        // Read in a single file
        string key = dm->loadFile(arg1, mcopy);
        if (key != "" && argc >= 1) {
            dm->displayData(key, frameNumber);
        } else if(key == "") {
            return proc_error;
        }

        //dm->addToRQ(arg1);

        return proc_complete;
    } else if (!S_ISDIR(fstat.st_mode)) {
	sprintf(str, "Not a file or directory: \"%.1024s\"", arg1);
	ib_errmsg(str);
	return proc_error;
    }        

    std::set<string> fileList;  // This sorts them alphabetically
    std::set<string>::iterator fitr;
    dm->makeImgList(arg1, fileList);

    if (fileList.empty()) {
	sprintf(str, "No FDF files in directory: \"%.1024s\"", arg1);
	ib_errmsg(str);
	return proc_error;
    }

    while (!fileList.empty()) {
        fitr = fileList.begin();
        // Open the file using the full path
        dm->loadFile(fitr->c_str(), mcopy);
        fileList.erase(fitr);
    }
    
    //dm->makeShortNames();
*/
    return proc_complete;
}

/*
 * Save updated header info.
 * Usage: aipSaveHeaders
 */
/* STATIC VNMRCOMMAND */
int
DataManager::aipSaveHeaders(int argc, char *argv[], int retc, char *retv[])
{
    // For now, no choice but to save them all.
    DataManager *dm = get();
    dm->saveAllHeaders();

    return proc_complete;
}

/**
 * Returns the number of headers not saved as a negative number.
 */
int
DataManager::saveAllHeaders()
{
    GframeManager *gfm __attribute__((unused));

    gfm = GframeManager::get();

    set<string> keys;
    keys = getKeys(DATA_SELECTED_FRAMES);
    if(keys.size() <= 0) keys = getKeys(DATA_ALL);

    int nFailed = 0;
    set<string>::iterator itr;
    for(itr=keys.begin(); itr!=keys.end(); ++itr) {

        spDataInfo_t di = getDataInfoByKey(*itr);
        if (di == (spDataInfo_t)NULL || !di->saveHeader()) {
            --nFailed;
        }
    }
    return nFailed;
}

int
DataManager::saveAllVs()
{
    GframeManager *gfm = GframeManager::get();

    set<string> keys;
    keys = getKeys(DATA_SELECTED_FRAMES);
    if(keys.size() <= 0) keys = getKeys(DATA_ALL);

    set<string>::iterator itr;
    spImgInfo_t image;
    for(itr=keys.begin(); itr!=keys.end(); ++itr) {
	spGframe_t gf = gfm->getCachedFrame(*itr);
        if (gf != nullFrame) {
	    image = gf->getFirstImage();
	    if(image != nullImg) {
		image->saveVsInHeader();
	    }
        }
    }

    int failures = saveAllHeaders();
    if (failures) {
        char msg[1000];
        const char *plural = "";
        if (failures < -1) {
            plural = "s";
        }

        sprintf(msg,"Failed to save %d header%s (permission problem?)",
                -failures, plural);
        ib_errmsg(msg);
    }
    return failures;
}


void
DataManager::makeImgList(const char *path, std::set<string>& list)
{
    DIR *dirp;
    struct dirent *dp;
    const int BUFLEN = 1024;
    char str[BUFLEN];
    struct stat fstat;
    const char *found;

    if(stat(path, &fstat) != 0) {
	sprintf(str, "%s: \"%.512s\"", strerror(errno), path);
	ib_errmsg(str);
	return;
    }

    found = strstr(path, ".fdf");
    if(S_ISREG(fstat.st_mode) && found && strlen(found) == 4) {
        list.insert(path);
	return;
    } else if (S_ISDIR(fstat.st_mode)) {

    // Open the directory
    dirp = opendir(path);
    if(dirp == NULL) {
	sprintf(str, "%s: \"%.512s\"", strerror(errno), path);
	ib_errmsg(str);
	return;
    }
    // Loop through the file list looking for fdf files.
    while ((dp = readdir(dirp)) != NULL && !interrupt()) {
	if (*dp->d_name != '.') {
	    sprintf(str, "%s/%s",path,dp->d_name);
	    makeImgList(str, list);	
	} 
    }
    closedir(dirp);

    }
}

void
DataManager::addToRQ(const char *path)
{
    // global param "rqactiontype" is created and init by RQaction.
    // data will be displayed after loading if is "DoubleClick"

    const int BUFLEN = 1024;
    char str[BUFLEN];
    char cmd[BUFLEN];

    if(P_getstring(GLOBAL, "rqactiontype", str, 1, sizeof(str))) 
       strcpy(str,"");

    sprintf(cmd,"RQ loadimgs %s imgstudy %s",path,str);
    writelineToVnmrJ("vnmrjcmd", cmd);
}

/*
 * Load data from file and display it in frame "frameNumber".
 * If "frameNumber" < 0, let gframeManager decide where to put it.
 */
string
DataManager::displayFile(const char *path, int frameNumber)
{
    int autoLayout = getReal("aipAutoLayout",0);
    GframeManager *gfm = GframeManager::get();

  if(autoLayout) {
    if (gfm->getNumberOfFrames() == 0) {
        gfm->splitWindow(1, 1);
    }

    if(frameNumber < 0) {
       frameNumber = gfm->getFirstAvailableFrame();
    }
    
    if(frameNumber >= gfm->getNumberOfFrames()) {
       double aspect = getAspectRatio();
       GframeManager::get()->splitWindow(frameNumber+1, aspect);
    }

    ReviewQueue::get()->loadFile(path,"",frameNumber+1,true);
    ReviewQueue::get()->display();
  } else {
    if (gfm->getNumberOfFrames() == 0) {
        // Make some frames to use
        int rows = (int)getReal("aipWindowSplit", 1, 1);
        int cols = (int)getReal("aipWindowSplit", 2, 1);
        gfm->splitWindow(rows,cols);
    }

    if(frameNumber < 0) {
       gfm->clearAllFrames();
    }
    
    ReviewQueue *rq = ReviewQueue::get();
    int batch = 1; 
    int nframes = gfm->getNumberOfFrames();
    int nimages = rq->getSelSize();
    rq->loadFile(path,"",nimages,true);
    if(nframes > 0) batch = (int)(0.5+(double)nimages/(double)nframes);
    if(batch < 1) batch = 1;
    if(nimages > (nframes*batch) && (nimages % nframes) != 0) batch += 1;
 
    rq->display(batch);
  }
    return string(path);
/*
    string key;
    if ((key = loadFile(path)) == "") {
	return key;
    }
    // fix bug: replace "//" if present in the key. 
    int pos = key.find("//");
    if(pos != string::npos) {
       key.replace(pos,2,"/");
    }

    ReviewQueue::get()->addImage(key, true);
    displayData(key, frameNumber);
    return key;
*/
}

/*
 * Display all data in appropriate frames.
 */
/* STATIC VNMRCOMMAND */
int
DataManager::aipDisplay(int argc, char *argv[], int retc, char *retv[])
{
    bool redisplay = false;
    bool sel = false;
    bool all = false;
    bool batch = false;
    bool reset = false;
    int cmdbits = 0;

    if(!WinMovie::get()->movieStopped() || !Movie::get()->movieStopped()) {
	Winfoprintf("Abort display: Movie is running.");	
        return proc_complete;
    }

    ReviewQueue *rq = ReviewQueue::get();
    argv++; argc--;
    for ( ; argc > 0; argv++, argc--) {
	if (strcasecmp(*argv, "redisplay") == 0) {
	    redisplay = true;
	}
	if (strcasecmp(*argv, "all") == 0) {
	    all = true;
	}
	if (strcasecmp(*argv, "sel") == 0) {
	    sel = true;
	}
	if (strcasecmp(*argv, "batch") == 0) {
	    batch = true;
	}
	if (strcasecmp(*argv, "show") == 0) {
	    cmdbits |= batchDisplay;
	}
	if (strcasecmp(*argv, "next") == 0) {
	    cmdbits |= batchNext;
	}
	if (strcasecmp(*argv, "previous") == 0) {
	    cmdbits |= batchPrevious;
	}
	if (strcasecmp(*argv, "first") == 0) {
	    cmdbits |= batchFirst;
	}
	if (strcasecmp(*argv, "last") == 0) {
	    cmdbits |= batchLast;
	}
	if (strcasecmp(*argv, "reset") == 0) {
	    reset = true;
	}
    }
    if (reset) {
        GframeManager::get()->clearFrameCache();
    }
    if (redisplay || VolData::get()->showingObliquePlanesPanel()) { 
	// refresh currently displayed images
        if (get()->getNumberOfImages() < 1) {
            // Nothing to redisplay; ignore completely
            return proc_complete;
        }
	GframeManager::get()->draw();
    } else if (all) {
        if(getReal("jviewport", 1) < 3) {
	    rq->displaySel(DATA_ALL, 1, 1);
	} else {
	    rq->displaySel(DATA_SELECTED_RQ, 1, 1);
	}
    } else if (batch) {
	rq->displayBatch(cmdbits); 
    } else if (sel) {
	rq->selectImages(DATA_SELECTED_FRAMES);
	rq->display(); 
    } else {
        if(getReal("jviewport", 1) < 3)
	   rq->selectImages(DATA_ALL);
	else
	   rq->selectImages(DATA_SELECTED_RQ);
	rq->display(); // redisplay currently selected images 
    }

    grabMouse();

    return proc_complete;
}

/*
 * Display just as many images as fit in the current number of frames.
 */
void
DataManager::displaySelected(int mode, int layout, int batch)
{
    grabMouse();

    list<string> selKeys = getKeylist(mode);
    int ni = selKeys.size();
    if(ni == 0) return;
 
    GframeManager *gfm = GframeManager::get();
    int nf = gfm->getNumberOfFrames();
    if(layout > 0 && nf != ni) {
	nf = ni;
	double aspect = getAspectRatio();
	gfm->splitWindow(nf, aspect);
        nf = gfm->getNumberOfFrames();

    } else if (nf < 1) {
	nf = (int)getReal("aipFrameDefaultMax", 3);
	int ni = getNumberOfImages();
	if (nf > ni) {
	    nf = ni;
	}
	double aspect = getAspectRatio();
	gfm->splitWindow(nf, aspect);
        nf = gfm->getNumberOfFrames();
    } else {
	gfm->clearAllFrames();
    }
    gfm->setFrameToLoad(0);

    int batches;
    if(ni%nf == 0)
        batches = ni/nf;
    else
	batches = ni/nf + 1;

    if(batch > batches) batch = 1;
    if(batch == 1) {
	setReal("aipBatches", batches, true);
	setReal("aipBatch", batch, true);
    }

    int low = (batch-1)*nf;
    int upp = (batch)*nf;
    int i;
    spDataInfo_t dataInfo;

    list<string>::iterator pt;
    for (i=0, pt = selKeys.begin(); pt != selKeys.end(); ++i, ++pt) {

        if(interuption) return;
	if(i < low) continue;
	if(i >= upp) break;

	dataInfo = DataManager::get()->getDataInfoByKey(*pt, true);
        if(dataInfo == (spDataInfo_t)NULL) continue;

	gfm->loadData(dataInfo);
    }

    VsInfo::setVsHistogram();
}

/*
 * Display all data in appropriate frames.
 */
void
DataManager::displayAll()
{
    GframeManager *gfm = GframeManager::get();
    gfm->deleteAllFrames();
    DataMap::iterator pd;

    grabMouse();

    // Make the right number and shape of frames.
    int nimages = getNumberOfImages();
    if (nimages < 1) {
	return;
    }
    // Get aspect ratio
    double aspect = getAspectRatio();
    gfm->splitWindow(nimages, aspect);

    // Make each image an appropriate ImageInfo
    // and put it in a Gframe.
    gfm->setFrameToLoad(0);	// Start loading in first frame
    //ImgInfo *imgInfo;
    for (pd = dataMap->begin(); pd != dataMap->end() && !interrupt(); pd++) {
        if(interuption) return;
        gfm->loadData(pd->second);
    }
    VsInfo::setVsHistogram();
}

/*
 * Display just as many images as fit in the current number of frames.
 */
void
DataManager::displayFirstN()
{
    //makeShortNames();
    //numberData();
    /*fprintf(stderr,"displayFirstN()\n"); */
    grabMouse();

    GframeManager *gfm = GframeManager::get();
    int nf = gfm->getNumberOfFrames();
    if (nf < 1) {
	nf = (int)getReal("aipFrameDefaultMax", 3);
	int ni = getNumberOfImages();
	if (nf > ni) {
	    nf = ni;
	}
	double aspect = getAspectRatio();
	gfm->splitWindow(nf, aspect);
    }
    int i;
    DataMap::iterator pd;
    for (i=0, pd = dataMap->begin();
         i<nf && pd != dataMap->end() && !interrupt();
         ++i, ++pd)
    {
        if(interuption) return;
        gfm->loadData(pd->second);
        /*{
            spGframe_t gf = GframeManager::get()->getFrameToLoad();
            fprintf(stderr,"load img #%d, row %d, col %d\n",
                    i, gf->row, gf->col);
        } */
    }
    VsInfo::setVsHistogram();
    /*fprintf(stderr,"displayFirstN done; "); */
    /*gfm->listAllFrames();  */
}

/*
 * Display a group of up to aipDisplay[2] images in the current frames.
 */
void
DataManager::displayBatch(int cmdbits)
{
    grabMouse();

    /*listAllData(); */

    // Get the current state of the display.
    int nImages = getNumberOfImages();
    GframeManager *gfm = GframeManager::get();
    int nFrames = gfm->getNumberOfFrames();
    if (nFrames == 0) {
        // Auto create frames if there are none
        int nf = (int)getReal("aipFrameDefaultMax", 3);
        nf = min(nf, nImages);
        gfm->splitWindow(nf, getAspectRatio());
        nFrames = gfm->getNumberOfFrames();
    }
    // NB: aipDisplay[1] is the image index starting from one (1, 2, ...),
    //     but all internal indices here start from zero (0, 1, ...).
    int firstImg = (int)getReal("aipDisplay", 1, 1) - 1; // 0, 1, ...
    if (firstImg < 0) {
        firstImg = 0;
    } else if (firstImg >= nImages) {
        firstImg = nImages - 1;
    }
    int batchSize = (int)getReal("aipDisplay", 2, 1);
    if (batchSize <= 0) {
        batchSize = nFrames;
    }
    int skip = (int)getReal("aipDisplay", 3, 1);
    skip = max(skip, 1);

    if (cmdbits & batchFirst) {
        firstImg = 0;
    }

    int indexInLap = firstImg / skip; // 0, 1, ...
    int firstInLap = firstImg % skip;
    int batchIndex = indexInLap / batchSize;
    int firstInBatch = firstInLap + batchIndex * batchSize * skip;
    int lastInBatch = firstInBatch + (batchSize - 1) * skip;

    if (cmdbits & batchLast) {
        firstInLap = skip - 1;
        indexInLap = (nImages - 1 - firstInLap) / skip; // of last image
        batchIndex = indexInLap / batchSize;
        firstInBatch = firstInLap + batchIndex * batchSize * skip;
        lastInBatch = firstInBatch + (batchSize - 1) * skip;
        int nb1 = (nImages + skip - 1 - firstInBatch) / skip - 1;
        nb1 = min(nb1, batchSize - 1);
        firstImg = firstInBatch + (nb1 / nFrames) * nFrames * skip;
    }

    if (cmdbits & batchNext) {
        // Set up to display the next group
        int numDisplayed = (lastInBatch - firstImg) / skip + 1;
        numDisplayed = min(nFrames, numDisplayed);
        if (firstImg + numDisplayed * skip >= nImages) {
            // Next image to display is in next lap
            firstInLap = (firstInLap + 1) % skip;
            firstImg = firstInLap;
            firstInBatch = firstInLap;
            lastInBatch = firstInBatch + (batchSize - 1) * skip;
            indexInLap = 0;
            batchIndex = 0;
        } else if (firstImg + numDisplayed * skip > lastInBatch) {
            // Next image to display is in next batch
            ++batchIndex;
            firstInBatch = firstInLap + batchIndex * batchSize * skip;
            lastInBatch = firstInBatch + (batchSize - 1) * skip;
            firstImg = firstInBatch;
            indexInLap = firstImg / skip;
            batchIndex = indexInLap / batchSize;
        } else {
            // Go to next part of this batch
            firstImg += numDisplayed * skip;
        }
    }

    if (cmdbits & batchPrevious) {
        // Set up to display the previous group
        if (firstImg == firstInLap) {
            // Previous bunch to display is in previous lap
            firstInLap = (firstInLap + skip - 1) % skip;
            indexInLap = (nImages - 1 - firstInLap) / skip; // of last image
            batchIndex = indexInLap / batchSize;
            firstInBatch = firstInLap + batchIndex * batchSize * skip;
            lastInBatch = firstInBatch + (batchSize - 1) * skip;
            int nb1 = (nImages + skip - 1 - firstInBatch) / skip - 1;
            nb1 = min(nb1, batchSize - 1);
            firstImg = firstInBatch + (nb1 / nFrames) * nFrames * skip;
        } else if (firstImg == firstInBatch) {
            // Previous bunch to display is in previous batch
            --batchIndex;
            firstInBatch = firstInLap + batchIndex * batchSize * skip;
            lastInBatch = firstInBatch + (batchSize - 1) * skip;
            int nb1 = (nImages + skip - 1 - firstInBatch) / skip - 1;
            nb1 = min(nb1, batchSize - 1);
            firstImg = firstInBatch + (nb1 / nFrames) * nFrames * skip;
        } else {
            // Go to previous part of this batch
            // NB: In this case, must have (nFrames < batchSize)
            firstImg -= nFrames * skip;
        }
    }

    if (cmdbits & batchDisplay) {
        // Display the current part of the current batch
        int img = max(firstImg, 0);
        /*fprintf(stderr,"Batch Display:\n"); */
        /*listAllData();  */
        for (int frame = 1; frame <= nFrames && !interrupt(); ++frame) {
            /*fprintf(stderr,"Display frame #%d:\n", frame); */
            /*listAllData(); */
            if (img < nImages && img <= lastInBatch) {
                displayData(img, frame-1);
            } else {
                spGframe_t gf = gfm->getFrameByNumber(frame);
                if (gf != nullFrame) {
                    gfm->replaceFrame(gf);
                }
            }
            img += skip;
        }
    }

    // Update parameter values
    setReal("aipDisplay", firstImg+1, true);
    VsInfo::setVsHistogram();
}

/*
 * Display data specified by key in a specified frame number (start from 0)
 */
bool
DataManager::displayData(const string key, int frameNumber)
{
    GframeManager *gfm = GframeManager::get();
    if (gfm->getNumberOfFrames() == 0) {
        // Make some frames to use
        int rows = (int)getReal("aipWindowSplit", 1, 1);
        int cols = (int)getReal("aipWindowSplit", 2, 1);
        gfm->splitWindow(rows, cols);
    }

    if(frameNumber < 0) {
       frameNumber = gfm->getFirstAvailableFrame();
    }
    
    if(frameNumber >= gfm->getNumberOfFrames()) {
       double aspect = getAspectRatio();
       GframeManager::get()->splitWindow(frameNumber+1, aspect);
    }

    gfm->setFrameToLoad(frameNumber);

    //grabMouse();
    
    spDataInfo_t dataInfo = DataManager::get()->getDataInfoByKey(key, true);
    if(dataInfo == (spDataInfo_t)NULL) { 
	fprintf(stderr, "displayData: Cannot find image, %s\n", key.c_str());
	return false;
    }
    bool rtn = gfm->loadData(dataInfo);
    return rtn;
}

/*
 * Display data image number in a specified frame number
 */
bool
DataManager::displayData(int imgNumber, int frameNumber)
{
    GframeManager *gfm = GframeManager::get();
    if (frameNumber >= 0) {
	gfm->setFrameToLoad(frameNumber);
    }

    if (imgNumber >= getNumberOfImages()) {
        return false;
    }
    bool rtn = gfm->loadData(getDataByNumber(imgNumber));
    //memTest();
    //grabMouse();
    //memTest();
    return rtn;
}

bool
DataManager::displayMovieFrame(const string key, bool fast)
{
    if (!fast) {
        grabMouse();
    }
    GframeManager *gfm = GframeManager::get();
    spGframe_t gf = gfm->getFrameByNumber(0);

    aip_movie_image(1);
    spDataInfo_t dataInfo = DataManager::get()->getDataInfoByKey(key, true);
    if(dataInfo == (spDataInfo_t)NULL) {
        fprintf(stderr, "displayMovieData: Cannot find image, %s\n", key.c_str());
        aip_movie_image(0);
        return false;
    }

    if(gfm->getCachedFrame(key) == nullFrame) aip_movie_image(0);
    bool rtn = gfm->loadData(dataInfo, gf);
    aip_movie_image(0);
    return rtn;
}

/*
 * Display data for Moviem specified by key in the first selected frame
 */

bool
DataManager::displayMovieData(const string key, bool fast)
{
    if (!fast) {
        grabMouse();
    }
    // aip_movie_image(0);
    GframeManager *gfm = GframeManager::get();
    GframeList::iterator gfi;
    spGframe_t gf = gfm->getFirstSelectedFrame(gfi);
    if (gf == nullFrame) {
	gf = gfm->getFirstFrame(gfi);
        if(gf != nullFrame) gf->setSelect(true, false);
    }
    if (gf == nullFrame) {
	gfm->splitWindow(1,1);	// Make one frame
	gf = gfm->getFirstFrame(gfi);
        if(gf != nullFrame) gf->setSelect(true, false);
    }
    if (gf == nullFrame) {
	return false;
    }

    // gf is the "first selected frame", which is different from
    // the "movie frame", although they occupy the same space.
    // this is because movie is displayed in a new frame that occupies
    // the same space of the "first selected frame". 
    // so after the first movie frame is displayed, gf is empty,
    // and movief is -1.

    int movief = gf->getGframeNumber();

    WinMovie *wm = WinMovie::get();
    int ownerf = wm->getOwnerFrame();
    spDataInfo_t ownerd = wm->getOwnerData();
    int guestf = wm->getGuestFrame();
    spDataInfo_t guestd = wm->getGuestData();

    //save owner frame if gf changed
    if(movief != -1 && movief != ownerf) {
        wm->restoreDisplay();
        // if movief happens to be the same as guestf, 
        // restoreDisplay will put guest frame to gf's location.
	// this action will empty gf. do the following to make
	// sure gf contains the actual frame, so the owner frame
	// can be properly saved.
	gf = gfm->getFrameByNumber(movief);
        gf->setSelect(true, false);

	spImgInfo_t img = gf->getFirstImage();
        if (img != nullImg) {
           wm->setOwnerFrame(movief, img->getDataInfo());
	}
    }

    // put image in gf back to guest frame
    if(guestf != ownerf && guestf > 0 && guestd != (spDataInfo_t)NULL) {
	 if(movief < 0)
         displayData(guestd->getKey(), guestf-1); 
    } 
    else if(ownerf > 0) {
// freeViewData of current frame (ownerf)
//fprintf(stderr,"freeViewData0 %d %d %d\n", movief, guestf, ownerf);
        spGframe_t f = gfm->getFrameByNumber(ownerf);
	if(f != nullFrame) {
	   spViewInfo_t view = f->getFirstView();
	   if(view != nullView) {
//fprintf(stderr,"freeViewData %s\n", view->imgInfo->getDataInfo()->getKey().c_str());
                gfm->unsaveFrame(view->imgInfo->getDataInfo()->getKey());
		f->saveRoi();
                f->clearFrame();
		view->freeViewData();	
	   }
	}
    }

    aip_movie_image(1);
    spDataInfo_t dataInfo = DataManager::get()->getDataInfoByKey(key, true);
    if(dataInfo == (spDataInfo_t)NULL) { 
	fprintf(stderr, "displayMovieData: Cannot find image, %s\n", key.c_str());
        aip_movie_image(0);
	return false;
    }
    //save guest frame
    guestf = gfm->getOwnerFrame(dataInfo->getKey())->getGframeNumber();
    wm->setGuestFrame(guestf, dataInfo);

    if(gfm->getCachedFrame(key) == nullFrame) aip_movie_image(0);
    bool rtn = gfm->loadData(dataInfo, gf);
    aip_movie_image(0);
    return rtn;
}

/*
 * Display data specified by key in the first selected frame
 */
bool
DataManager::displayData(const string key, bool fast)
{
    if (!fast) {
        grabMouse();
    }

    GframeManager *gfm = GframeManager::get();
    GframeList::iterator gfi;
    spGframe_t gf = gfm->getFirstSelectedFrame(gfi);
    if (gf == nullFrame) {
	gf = gfm->getFirstFrame(gfi);
    }
    if (gf == nullFrame) {
	gfm->splitWindow(1,1);	// Make one frame
	gf = gfm->getFirstFrame(gfi);
    }
    if (gf == nullFrame) {
	return false;
    }

    spDataInfo_t dataInfo = DataManager::get()->getDataInfoByKey(key, true);
    if(dataInfo == (spDataInfo_t)NULL) { 
	fprintf(stderr, "displayData: Cannot find image, %s\n", key.c_str());
	return false;
    }
    bool rtn = gfm->loadData(dataInfo, gf);
    return rtn;
}

DataMap *
DataManager::getDataMap() 
{
    return dataMap;
}

/**
 * Return a list of the currently loaded ScanGroups.
 */
set<string>
DataManager::getGroups()
{
    // Make collection of unique groups
    set<string> groups;
    DataMap::iterator pd;
    for (pd = dataMap->begin(); pd != dataMap->end(); ++pd) {
        groups.insert(pd->second->getGroup());
    }
    return groups;
}

string
DataManager::getFirstSelectedKey()
{
    string key = "";
    GframeManager *gfm = GframeManager::get();
    GframeList::iterator gfi;
    spGframe_t gf=gfm->getFirstSelectedFrame(gfi);
    if(gf != nullFrame)
    {
	spViewInfo_t vi;
	if ((vi=gf->getFirstView()) != nullView) {
	    key = vi->imgInfo->getDataInfo()->getKey();
	}
    }
    return key;
}

list<string>
DataManager::getKeylist(int mode)
{
   return ReviewQueue::get()->getKeylist(mode);
}

set<string>
DataManager::getKeys(int mode, string strArg)
{
   return ReviewQueue::get()->getKeyset(mode, strArg);
}

void
DataManager::makeShortNames()
{
    list<string> paths;
    DataMap::iterator pd;
    string::size_type idx;
  
    for (pd = dataMap->begin(); pd != dataMap->end(); ++pd) {
        paths.push_back(string(pd->second->getFilepath()));
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
            for (j = i; j < (int) (idx - 1) && pname->at(j) == '0'; ++j);
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

    // Save abbreviated names in the DataInfo 
    for (pd = dataMap->begin(), pname = paths.begin(); 
	  pd != dataMap->end() && pname != paths.end(); ++pd, ++pname) {
        pd->second->setShortName(*pname);
    }
}

void
DataManager::listAllData()
{
    DataMap::iterator pd;

    for (pd = dataMap->begin();
         pd != dataMap->end();
         ++pd)
    {
        fprintf(stderr,"%s: width=%d\n", pd->second->getFilepath(),
                pd->second->getFast());
    }
}

/*
 * get value from FDF header 
 */
/* STATIC VNMRCOMMAND */
int
DataManager::aipGetHeaderParam(int argc, char *argv[], int retc, char *retv[])
{
/* 
  key is fullpath to img dir + " " + fdf filename
  name is param name
*/
    if(retc <= 0) return proc_complete;
 
    string key = "";
    string name = "";
    int idx = -1;
    if (argc == 4) {
        key = argv[1];
        name = argv[2];
        idx = atoi(argv[3]) - 1;
    } else if (argc == 3 && argv[1][0] == '/') {
        key = argv[1];
        name = argv[2];
    } else if (argc == 3) {
        key = get()->getFirstSelectedKey();
        name = argv[1];
        idx = atoi(argv[2]) - 1;
    } else if (argc == 2) {
        key = get()->getFirstSelectedKey();
        name = argv[1];
    } else {
	return proc_error;
    }
    
    string str = get()->getHeaderParam(key, name, idx);
    
    string type = "";
    string value = "";
    unsigned int p = str.find(" ", 0);
    if(p != string::npos) {
      type = str.substr(0,p);
      value = str.substr(p+1,str.length()-1); 
    }
    retv[0] = newString(value.c_str());
    if(retc>1) retv[1] = newString(type.c_str());
    return proc_complete;
}

string 
DataManager::getHeaderParam(string key, string name, int idx)
{
    char  *sval;
    int  ival;
    double  dval;
    char str[MAXSTR];

    if(getHeaderStr(key, name, idx, &sval)) {
        if (sval != NULL) {
	   sprintf(str, "str %s",sval);
	   return string(str);
	}
    }

    //try as first elem of array
    if(idx != -1 && getHeaderStr(key, name, -1, &sval)) {
        if (sval != NULL) {
	   sprintf(str, "str %s",sval);
	   return string(str);
	}
    }
    if(getHeaderStr(key, name, 0, &sval)) {
        if (sval != NULL) {
	   sprintf(str, "str %s",sval);
	   return string(str);
	}
    }

    if(getHeaderReal(key, name, idx, &dval)) {
	sprintf(str, "real %f",dval);
	return string(str);
    }

    if(idx != -1 && getHeaderReal(key, name, -1, &dval)) {
	sprintf(str, "real %f",dval);
	return string(str);
    }
    if(getHeaderReal(key, name, 0, &dval)) {
	sprintf(str, "real %f",dval);
	return string(str);
    }

    if(getHeaderInt(key, name, idx, &ival)) {
	sprintf(str, "int %d",ival);
	return string(str);
    }

    if(idx != -1 && getHeaderInt(key, name, -1, &ival)) {
	sprintf(str, "int %d",ival);
	return string(str);
    }
    if(getHeaderInt(key, name, 0, &ival)) {
	sprintf(str, "int %d",ival);
	return string(str);
    }
    return "";
}

int
DataManager::aipGetHeaderString(int argc, char *argv[], int retc, char *retv[])
{
/* 
  key is fullpath to img dir + " " + fdf filename
  name is param name
*/
    if(retc <= 0) return proc_complete;
 
    string key = "";
    string name = "";
    int idx = -1;
    if (argc == 4) {
        key = argv[1];
        name = argv[2];
        idx = atoi(argv[3]);
    } else if (argc == 3 && argv[1][0] == '/') {
        key = argv[1];
        name = argv[2];
    } else if (argc == 3) {
        name = argv[1];
        idx = atoi(argv[2]);
    } else if (argc == 2) {
        name = argv[1];
    } else {
	return proc_error;
    }

    char *value;
    if(get()->getHeaderStr(key, name, idx, &value)) {
        retv[0] = newString(value);
    }
    return proc_complete;
}

/*
 * get value from FDF header 
 */
/* STATIC VNMRCOMMAND */
int
DataManager::aipGetHeaderInt(int argc, char *argv[], int retc, char *retv[])
{
/* 
  key is fullpath to img dir + " " + fdf filename
  name is param name
*/
    if(retc <= 0) return proc_complete;
 
    string key = "";
    string name = "";
    int idx = -1;
    if (argc == 4) {
        key = argv[1];
        name = argv[2];
        idx = atoi(argv[3]);
    } else if (argc == 3 && argv[1][0] == '/') {
        key = argv[1];
        name = argv[2];
    } else if (argc == 3) {
        name = argv[1];
        idx = atoi(argv[2]);
    } else if (argc == 2) {
        name = argv[1];
    } else {
	return proc_error;
    }

    int value = 0;
    if(get()->getHeaderInt(key, name, idx, &value)) {
        retv[0] = realString((double)value);
    }
    return proc_complete;
}

/*
 * get value from FDF header 
 */
/* STATIC VNMRCOMMAND */
int
DataManager::aipGetHeaderReal(int argc, char *argv[], int retc, char *retv[])
{
/* 
  key is fullpath to img dir + " " + fdf filename
  name is param name
*/
    if(retc <= 0) return proc_complete;
 
    string key = "";
    string name = "";
    int idx = -1;
    if (argc == 4) {
        key = argv[1];
        name = argv[2];
        idx = atoi(argv[3]);
    } else if (argc == 3 && argv[1][0] == '/') {
        key = argv[1];
        name = argv[2];
    } else if (argc == 3) {
        name = argv[1];
        idx = atoi(argv[2]);
    } else if (argc == 2) {
        name = argv[1];
    } else {
	return proc_error;
    }

    double value = 0.0;
    if(get()->getHeaderReal(key, name, idx, &value)) {
        retv[0] = realString(value);
    }
    return proc_complete;
}

bool
DataManager::getHeaderStr(string key, string name, int idx, char **value)
{
    if(name == "") return false;

    spDataInfo_t dataInfo = getDataInfoByKey(key);
    if(dataInfo == (spDataInfo_t)NULL) return false;

    const char *cname = name.c_str();
    if(idx != -1) {
      if(dataInfo->st->GetValue(cname,*value,idx)) {
	return true;
      } else {
	return false;
      }
    } else {
      if(dataInfo->st->GetValue(cname,*value)) {
	return true;
      } else {
	return false;
      }
    }
}

bool
DataManager::getHeaderInt(string key, string name, int idx, int *value)
{  
    if(name == "") return false;

    spDataInfo_t dataInfo = getDataInfoByKey(key);
    if(dataInfo == (spDataInfo_t)NULL) return false;

    const char *cname = name.c_str();
    if(idx != -1) {
      if(dataInfo->st->GetValue(cname,*value,idx)) {
	return true;
      } else {
	return false;
      }
    } else {
      if(dataInfo->st->GetValue(cname,*value)) {
	return true;
      } else {
	return false;
      }
    }
}
    
bool
DataManager::getHeaderReal(string key, string name, int idx, double *value)
{  
    if(name == "") return false;

    spDataInfo_t dataInfo = getDataInfoByKey(key);
    if(dataInfo == (spDataInfo_t)NULL) return false;

    const char *cname = name.c_str();
    if(idx != -1) {
      if(dataInfo->st->GetValue(cname,*value,idx)) {
	return true;
      } else {
	return false;
      }
    } else {
      if(dataInfo->st->GetValue(cname,*value)) {
	return true;
      } else {
	return false;
      }
    }
}

spDataInfo_t
DataManager::getDataInfoByKey(string key, bool load) 
{
    if(key == "") {
	 return (spDataInfo_t)NULL;
    }

    DataMap::iterator pd = dataMap->find(key);
    if (pd == dataMap->end() && load) {
        ReviewQueue::get()->loadKey(key);
	pd = dataMap->find(key);
    }
    if (pd == dataMap->end()) {
	fprintf(stderr, "getDataInfoByKey: Cannot find image, %s\n", key.c_str());
    	return (spDataInfo_t)NULL;
    }
    return pd->second;
}

void
DataManager::deleteDataByKey(string key)
{
    DataMap::iterator pd = dataMap->find(key);
    if (pd != dataMap->end()) {
        dataMap->erase(key); // Delete data from our list
        WinMovie::dataMapChanged();
    } else return;

    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;
    GframeList::iterator gfi;
    for (gf=gfm->getFirstFrame(gfi); 
	gf != nullFrame; 
	gf=gfm->getNextFrame(gfi)) {
	spViewInfo_t vi;
	if ((vi=gf->getFirstView()) != nullView) {
	    string newkey = vi->imgInfo->getDataInfo()->getKey();
	    if(newkey == key) { 
                gfm->unsaveFrame(key);
                gf->clearFrame();
                if (aipHasScreen()) {
                   gf->draw();
                }
	    }
	}
    }

    RoiStat::get()->calculate();
    VsInfo::setVsHistogram();
    //makeShortNames();
    //numberData();
}

/* STATIC VNMRCOMMAND */
int
DataManager::aipNumOfImgs(int argc, char *argv[], int retc, char *retv[])
{
    if(retc > 0) {
        retv[0] = realString((double)get()->dataMap->size());
    }
    return proc_complete;
}

/* STATIC VNMRCOMMAND */
int
DataManager::aipNumOfCopies(int argc, char *argv[], int retc, char *retv[])
{
// used to set num of copies when build RQ treetable xml file.
// findCopiesInDataMap returns number of copies, 
// but here copies - 1 (the index) is returned. 
    if(argc < 2) return proc_error;
    if(retc < 1) return proc_complete;

    int copies = DataManager::get()->findCopiesInDataMap(argv[1]);
    retv[0] = realString((double)(copies));

    return proc_complete;
}

/* STATIC VNMRCOMMAND */
int
DataManager::aipLoadImgList(int argc, char *argv[], int retc, char *retv[])
{
    if(argc < 2) return proc_error;

    char  buf[MAXSTR];
    FILE *fp = fopen(argv[1], "r");
     
    if(!fp) return proc_error;

    DataManager *dm = DataManager::get();
    
    while (fgets(buf,sizeof(buf),fp)) {
        if(strlen(buf) > 1 && buf[0] != '#' && buf[0] == '/') {
	    strcpy(strstr(buf,"\n"), ""); 
	    dm->loadFile(buf);
        }
    }

    fclose(fp);
    unlink(argv[1]);
	return proc_complete;
}

int
DataManager::aipGetImgKey(int argc, char *argv[], int retc, char *retv[])
{
     if(retc < 1 || argc < 2) return proc_complete;

     DataManager *dm = DataManager::get();

     if(argc < 3) {
	selectedSet.clear();
        int mode = atoi(argv[1]);
        std::list<string> slist = dm->getKeylist(mode);
	std::list<string>::iterator sitr;
	for (sitr = slist.begin(); sitr != slist.end(); ++sitr) {
		selectedSet.insert(*sitr);
	}
        retv[0] = realString((double)(selectedSet.size()));
	return proc_complete;
     } else {
	int i; 
        int ind = atoi(argv[2]); 
	std::set<string>::iterator sitr;
	for (i=1, sitr = selectedSet.begin(); sitr != selectedSet.end(); ++sitr, ++i) {
	    if(i == ind) {
		retv[0] = newString(sitr->c_str());
		return proc_complete;
	    }
	}
     }

     return proc_error;
}

/*
 * Write out the 3D file converted from the 2D loaded slices
 */
void 
DataManager::aip2Dto3Dwork(char *outfile) {
    DataMap::iterator pd;
    spDataInfo_t info;
    dataStruct_t *ds;
    const char *fullpath=NULL, *filename=NULL, *inpath;
    string filestring, tmpname;
    char error[200], cmd[200];
    const char *parent_c;
    int fast, med, rank;
    string::size_type idx;
    string strpath, oldParent, newParent;
    char path[200], name[100];

    // If outfile is not empty, use it
    if(outfile[0] != '\0') {
        // We have an outfile given as a arg.  If it starts with a /, then
        // use it as a fullpath, else as an output filename in the current dir
        if(outfile[0] == '/') {
            // We have a fullpath, use it.  Be sure it ends in .fdf and
            // add it if necessary
            filestring = outfile;
            string::size_type idx = filestring.find(".fdf");
            if(idx == string::npos) {
                // It does not contain .fdf, add it
                tmpname = filestring + ".fdf";
                fullpath = tmpname.c_str();
            }
            else {
                // It already has .fdf, just use this name
                fullpath = outfile;
            }
            
            // Get the filename, including the .fdf
            filestring = fullpath;
            // Now find last slash and get from there to end including .fdf
            idx = filestring.find_last_of("/");
            tmpname = filestring.substr(idx+1);
            filename = tmpname.c_str();
        }
        else {
            // No starting  /, must not be a fullpath.  Therefore it should
            // be a filename.  Be sure it has .fdf
            filestring = outfile;
            string::size_type idx = filestring.find(".fdf");
            if(idx == string::npos) {
                // It does not contain .fdf, add it
                tmpname = filestring + ".fdf";
                filename = tmpname.c_str();
            }
            else {
                // It already has .fdf, just use this name
                filename = outfile;
            }            
        } 
    }
    if(fullpath == NULL) {
        // We did not get a fullpath as an arg, make one using the input
        // files name.
        if(dataMap->begin() == dataMap->end()) {

            // No directory available, use curexp/datadir3d/data if it exists
            strcpy(path, curexpdir);
            // First create the parent directory and save that
            strcat(path,  "/datadir3d/data/");
            newParent = path;
            strcpy(name, "pseudo3D.fdf");
            filename = name;
            // Then create the full path with filename
            strcat(path,  filename);
            fullpath = path;
        }
        else {
            pd = dataMap->begin();
            info = pd->second;
            // inpath will be the path of the .fdf file
            inpath = info->getFilepath();
            // We need to trim off the current .fdf name to get the parent.
            strpath = inpath;
            idx = strpath.find_last_of("/");
            if(idx != string::npos) {
                // we want to keep from the beginning up to idx
                newParent = strpath.substr(0, idx);
                
                // Now we have a parent dir defined, create the fullpath
                strpath = newParent;
                strpath.append("/");
                if(filename == NULL)
                    filename = "pseudo3D.fdf";
                strpath.append(filename);
                fullpath = strpath.c_str();
            }
            else { 
                sprintf(error, "Problem with path: %s", inpath);
                ib_errmsg(error);
                
                
                
                return;
            }
        }
    }
    else {
    	// fullpath must have been given.  We need to be sure the
    	// parent exist and if not make it below.  Create the parent here.
    	strpath = fullpath;
    	idx = strpath.find_last_of("/");
        if(idx != string::npos) {
            // we want to keep from the beginning up to idx
            newParent = strpath.substr(0, idx);
        }
    }
   
    // We should have a parent and a fullpath by the time we arrive here.
    // See if the parent exist and if not, try to create it.
    struct stat buf;
    // If parent does not exist, we need to make the dir, look at 
    // another level up also
    idx = newParent.find_last_of("/");
    string parentParent = newParent.substr(0, idx);
    int status = stat(parentParent.c_str(), &buf);
    if(status == -1) {
        // the directory must not exist, try to make it.
        parent_c = parentParent.c_str();
        int status = mkdir(parent_c, 0755);
        if(status == -1) {
            sprintf(error, "%s: Could not create %s", strerror(errno), parent_c);
            ib_errmsg(error);
            return;
        }
    }
    
    status = stat(newParent.c_str(), &buf);
    if(status == -1) {
        // the directory must not exist, try to make it.
        parent_c = newParent.c_str();
        int status = mkdir(parent_c, 0755);
        if(status == -1) {
            sprintf(error, "%s: Could not create %s", strerror(errno), parent_c);
            ib_errmsg(error);
            return;
        }
       
        // If we had to create a parent directory, it will need a procpar
        // See if one exists in oldParent and if so, copy it to the 
        // newParent directory.
        char procparPath[200];
        sprintf(procparPath, "%s/procpar", oldParent.c_str());
        status = stat(procparPath, &buf);
        if(status == -1) {
            // ****** WARNING, no procpar
        }
        else {
            // Okay, we have a procpar to use, copy it to the newParent dir
            char buf[512];
            int n, fin, fout;
     	    char procparNew[200];
            int ret __attribute__((unused));
            sprintf(procparNew, "%s/procpar", newParent.c_str());
         	
            // Just manually create and copy the contents
            fin = open(procparPath, 0);
            fout = creat(procparNew, 0644);
     	    while((n=read(fin, buf, 512)) > 0) {
     	    	ret = write(fout, buf, n);
     	    }
            close(fin);
            close(fout);
        }
    }
    

    if(fullpath == NULL) {
        ib_errmsg("Problem with path for saving 3D .fdf file");
        return;
    }
    
    // Create the 3d header and write into the file
    create3Dheader(fullpath, filename);

    // Open the output file which already has the header written
    FILE *file = fopen(fullpath, "a");
    	
    float *datatemp = NULL;
    if(file != NULL) {
        // Loop through the slices if the file was successfully opened
        for (pd = dataMap->begin(); pd != dataMap->end() && !interrupt(); pd++) {
        	// pd->second gives the structure with the data pointer for 
    	    // each slice
            info = pd->second;
            ds = info->dataStruct;
            fast = info->getFast();
            med = info->getMedium();
			rank = info->getRank();
			// Get size of each slice
			// size = fast * med;
			if (rank == 2)
				fwrite(ds->data, fast * med, sizeof(float), file);

		}
        fclose(file);
        if(datatemp)
        	(void)free(datatemp);

        parent_c = newParent.c_str();

        // Put the .img directory into the DB
        // Cause the locator to update after this addition
        sprintf(cmd, "LOC add image_dir %s updatelocdis", parent_c);
        writelineToVnmrJ("vnmrjcmd", cmd);
    }
}

/*
 * Take the loaded slices and create a 3D .fdf file from them.
 */
/* STATIC VNMRCOMMAND */
int
DataManager::aip2Dto3D(int argc, char *argv[], int retc, char *retv[])
{
    char filepath[200] = "";
    if(argc > 1) {
        /* Get optional filename arg if given */
        strcpy(filepath, argv[1]);
    }

    // The current method is static.  I need to call the non static 
    // method to do the work.
    get()->aip2Dto3Dwork(filepath);

    return proc_complete;


}

/* 
 * Create the header for the fdf file and write it into the file
 * pointer given.
 */
void DataManager::create3Dheader(const char *filepath, const char *filename) {

    DataMap::iterator pd;
    spDataInfo_t info;
    DDLSymbolTable *st;
    dataStruct_t *ds;
    const char *firstpath;
    string strpath, strfirst;
    char error[500];
    int i;
    // fast is the num points in a row, med is number of rows in a plane
    // slow is the number of planes in a cube
    int fast, med, slow;
    int rank, slices;
    string::size_type idx;
    double location_first, location_last, span, loc3d, orig, roi3d;
    float frot[9];
    double rot[9];
    float aphi, apsi, atheta;
	float cospsi, cosphi, costheta;
	float sinpsi, sinphi, sintheta;
	double or0, or1, or2, or3, or4, or5, or6, or7, or8;
	const float eps = 1.0e-4;

	// We need the parent of the first slice for comparison below
    pd = dataMap->begin();
    info = pd->second;
    firstpath = info->getFilepath();
    strfirst = firstpath;
    // get the parent
    idx = strfirst.find_last_of("/");
    strfirst = strfirst.substr(0, idx);

    // We need the size of the first slice to compare below
    fast = info->getFast();
    med  = info->getMedium();
    
    // How many slices do we have?  There may be an iterator method
    // to get this, but I don't know it so I will just count them.
    // While we are at it, check that each slice is the same size.
    for(i=0, pd=dataMap->begin(); pd != dataMap->end(); pd++, i++) { 
        info = pd->second;
 	rank = info->getRank();
	if(rank != 2)i--;
        if(fast != info->getFast() || med != info->getMedium()) {
            ib_errmsg("Cannot mix different slices of different dimensions, Aborting.");
            return;
        }
    }
    slow = i;
    slices=slow;

    // Get the location of the last slice.  That should currently be in 'info'
    info->st->GetValue("location", location_last, 2);

    // Get some info from the first slice (and skip over any non-2D slices)
    pd = dataMap->begin();
    info = pd->second;
    rank = info->getRank();
    while(rank != 2)
      {
		pd++;
		info = pd->second;
		rank = info->getRank();
	}

    // Make a clone of the symbol table since we will be modifying it
    st = (DDLSymbolTable *)info->st->CloneList(false);

    ds = info->dataStruct;
    // fullpath = info->getFilepath();
    fast = info->getFast();
    med  = info->getMedium();
    rank = info->getRank();

    // swap first and second values for matrix, span, origin, location, roi
    /*
    st->GetValue("matrix", dtempa, 0);
	st->GetValue("matrix", dtempb, 1);
	st->SetValue("matrix", dtempb, 0);
	st->SetValue("matrix", dtempa, 1);

	st->GetValue("span", dtempa, 0);
	st->GetValue("span", dtempb, 1);
	st->SetValue("span", dtempb, 0);
	st->SetValue("span", dtempa, 1);

	st->GetValue("origin", dtempa, 0);
	st->GetValue("origin", dtempb, 1);
	st->SetValue("origin", dtempb, 0);
	st->SetValue("origin", dtempa, 1);

	st->GetValue("location", dtempa, 0);
	st->GetValue("location", dtempb, 1);
	st->SetValue("location", dtempb, 0);
	st->SetValue("location", dtempa, 1);

	st->GetValue("roi", dtempa, 0);
	st->GetValue("roi", dtempb, 1);
	st->SetValue("roi", dtempb, 0);
	st->SetValue("roi", dtempa, 1);
	*/

    // Change the necessary header values
    st->SetValue("rank", 3);
    st->SetValue("spatial_rank", "3dfov");
    st->SetValue("subrank", "3dfov");
    st->SetValue("matrix", slow, 2);
    st->SetValue("abscissa", "cm", 2);
#ifdef LINUX
    st->SetValue("bigendian", 0.0);
#else
    st->SetValue("bigendian", 1);
#endif
    st->SetValue("slices", slow);

    // span should = location[2]last_slice - location[2]first_slice + one thickness
    // the thickness is because the locations are the centers of the slices given
    // and the span is edge to edge.
    double thick = ds->roi[2];
    // location of the first slice
    
    // 2D input, calc span from locations
    st->GetValue("location", location_first, 2);
    span = location_last - location_first + thick;
    loc3d = .5*(location_first + location_last);
    orig = loc3d - 0.5*(thick*slices);
    roi3d = slices*thick;

    st->SetValue("location", loc3d, 2);
    st->SetValue("span", span, 2);
    st->SetValue("roi", roi3d, 2);
    st->SetValue("origin", orig, 2);
    st->SetValue("file", filepath);



    // get orientation matrix and convert to 3D fdf form
    // first figure out the euler angles
	info->GetOrientation(rot);
	for(i=0;i<9;i++)
		frot[i]=(float)rot[i];
	tensor2euler(&atheta, &apsi, &aphi, frot);

	aphi=aphi+90.; // rotate 90 ro & pe channels
	snapAngle(&atheta);
	snapAngle(&apsi);
	snapAngle(&aphi);

	cospsi = cosf(DEG_TO_RAD * apsi);
	sinpsi = sinf(DEG_TO_RAD * apsi);
	cosphi = cosf(DEG_TO_RAD * aphi);
	sinphi = sinf(DEG_TO_RAD * aphi);
	costheta = cosf(DEG_TO_RAD * atheta);
	sintheta = sinf(DEG_TO_RAD * atheta);

	// the 3D form of the rotation matrix
	or0 = -1 * sinphi * sinpsi - cosphi * costheta * cospsi;
	or1 = sinphi * cospsi - cosphi * costheta * sinpsi;
	or2 = sintheta * cosphi;
	or3 = cosphi * sinpsi - sinphi * costheta * cospsi;
	or4 = -1 * cosphi * cospsi - sinphi * costheta * sinpsi;
	or5 = sintheta * sinphi;
	or6 = cospsi * sintheta;
	or7 = sinpsi * sintheta;
	or8 = costheta;

	or0=eps*ROUND(or0/eps);
	or1=eps*ROUND(or1/eps);
	or2=eps*ROUND(or2/eps);
	or3=eps*ROUND(or3/eps);
	or4=eps*ROUND(or4/eps);
	or5=eps*ROUND(or5/eps);
	or6=eps*ROUND(or6/eps);
	or7=eps*ROUND(or7/eps);
	or8=eps*ROUND(or8/eps);



    st->SetValue("orientation", or0, 0);
    st->SetValue("orientation", or1, 1);
    st->SetValue("orientation", or2, 2);
    st->SetValue("orientation", or3, 3);
    st->SetValue("orientation", or4, 4);
    st->SetValue("orientation", or5, 5);
    st->SetValue("orientation", or6, 6);
    st->SetValue("orientation", or7, 7);
    st->SetValue("orientation", or8, 8);

	// Get the file name without its .fdf extension
	string name = filename;
	idx = name.find(".fdf");
	if (idx != string::npos)
		name = name.substr(0, idx);

	st->SetSrcid(name.c_str());
	st->SetValue("filename", name.c_str());

  

    ofstream fout;
    fout.open(filepath);
    
    if (!fout) {
        sprintf(error, "%s: %s", strerror(errno), filepath);
        ib_errmsg(error);
        return;
    }

    // PrintSymbols does not write the first line needed, so write it.
    fout << "#!/usr/local/fdf/startup";
    // Write out the st symbol table as modified
    st->PrintSymbols(fout);
    
    // Pad the length of the header out to an even word boundary
    // Get the current write position which is one past the end of
    // the current stream.  We want that extra 1 left in because we
    // will write out a \0 after the padding
    int length = fout.tellp();
	int align = sizeof(float);
	int pad_cnt = length % align;
  	for(i=0;i<pad_cnt;i++) {
    	fout << '\n';
    }	
	
	// append on the terminating \0 required.
    fout << endl << '\0';
	
    fout.close();

}

void DataManager::erasePlane(char *orient) {
    DataMap::iterator pd;
    string key;
    for (pd = dataMap->begin(); pd != dataMap->end(); ++pd) {
        key = pd->first;
	if(key.find(orient) != string::npos)
          dataMap->erase(key);	// Delete any old version.
    }
} 
