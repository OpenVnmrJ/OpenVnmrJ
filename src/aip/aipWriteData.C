/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>

#include "aipCommands.h"
#include "aipVnmrFuncs.h"
using namespace aip;
#include "aipUtils.h"
#include "aipGframe.h"
#include "aipImgInfo.h"
#include "aipGframeManager.h"
#include "aipWriteData.h"

#define ByteSwap5(x) ByteSwap((unsigned char *) &x,sizeof(x))
extern void ByteSwap(unsigned char * b, int n);

/*
 * Writes image data of all selected images.  Returns path to last
 * file written if successful. Otherwise, returns "".
 * If one arg, it should be fullpath including fdf base name
 * If two args, 1st is .img directory name and 2nd is fdf base name
 * STATIC VNMRCOMMAND
 */
int
WriteData::writeData(int argc, char *argv[], int retc, char *retv[])
{
    GframeManager *gfm = GframeManager::get();
    GframeList::iterator gfi;

    // Get a frame
    spGframe_t gf = gfm->getFirstSelectedFrame(gfi);
    bool selected = gf != nullFrame;
    if (!selected) {
        gf = gfm->getFirstFrame(gfi);
    }
    spImgInfo_t img;
    bool gotSomething = false;
    if (gf != nullFrame) {
        gotSomething = true;
	img = gf->getFirstImage();
    }

    // Get first frame with an image in it
    while (gf != nullFrame && img == nullImg) {
        if (selected) {
            gf = gfm->getNextSelectedFrame(gfi);
        } else {
            gf = gfm->getNextFrame(gfi);
        }
	if (gf != nullFrame) {
	    img = gf->getFirstImage();
	}
    }
    if (gf == nullFrame) {
	gotSomething = false;
    }

    string tpath;
    if (gotSomething) {
        bool fdf = false;
	char fmtstr[MAXSTR];
	strcpy(fmtstr,getString("aipWriteFmtConvert", "FDF").c_str());
        
        // Get filepath to save
        string path;
        if (strncasecmp(fmtstr,"FDF",3) == 0) {
            fdf = true;
        }
        if (argc == 1) {
            //If no args, get pathfrom parameter
            path = getString("aipWritePath", "");
        }
        else if (argc == 2) {
            // If one arg, it should be fullpath including base fdf name
            path = argv[1];
        }
        else if (argc == 3) {
            // Two args, must have directory and filename separately
            string dir = argv[1];
            string name = argv[2];
            
            // If dir is empty, default to something
            if(dir.size() == 0)
                dir = "default";
            
            if(name.size() == 0)
                name = "info";
 
            // If dir does not end in .img, add it
            string::size_type index = dir.find(".img");
            if (index == string::npos)
                dir.append(".img");
                       
            // Create the path with dir and name args
            path = dir + "/" + name;
        }

        if (path.size() == 0) {
            fprintf(stderr,"Null file name for image data output\n");
        } else {
            // Maybe add a base path
            if (path[0] != '/') {
                // If not a fullpath, set the dir to  'sqdir' + data
                // If we should do something different for plan, current
                // and review viewports, see the macro xm1 $1='vp'
                // for how it determines the viewport type.
                path = getString("sqdir", "") + "/data/" + path;
            }

            // If the directory does not exist, try to create it
            // Do this for .img directory and the level above that
            
            // Strip off filename get just the directory, find last '/'
            string::size_type idx = path.rfind("/");
 
            string dir1 = path.substr(0, idx);

            // Get next dir above dir1
            idx = dir1.rfind("/");
            string dir2 = dir1.substr(0, idx);

            // Make dir above .img directory if needed
            if(access(dir2.c_str(), R_OK) != 0) {
                // It must not exist, try to make it
                char str[256];
                (void)sprintf(str, "mkdir %s \n", dir2.c_str());
                (void)system(str);
            }
            // Make .img directory if needed
            if(access(dir1.c_str(), R_OK) != 0) {
                // It must not exist, try to make it
                char str[256];
                (void)sprintf(str, "mkdir %s \n", dir1.c_str());
                (void)system(str);
            }
            
            // Do we now have a writable directory ready to go?
            if(access(dir1.c_str(), W_OK) != 0) {
                char str[512];
                (void)sprintf(str, "Problem writing to %s\n", dir1.c_str());
                ib_errmsg(str);
                return proc_error;
            }

            // Put index number on end of file name
            // Look for period only in last few chars.  Else, it finds a period
            // in the directory name like name.img
            string::size_type len = path.size();
            idx = path.find(".", len-5); // Where to put index nbr
            if (idx == string::npos) {
                path += ".";
                idx = path.size();
            } else {
                path.insert(idx, ".0000");
                ++idx;
            }

            // Write out all the files
            int index = 1;
            for (gf = selected
                         ? gfm->getFirstSelectedFrame(gfi)
                         : gfm->getFirstFrame(gfi);
                 gf != nullFrame;
                 gf = selected
                         ? gfm->getNextSelectedFrame(gfi)
                         : gfm->getNextFrame(gfi)
                 )
            {
                if (gf->getFirstImage() != nullImg) {
                    char buf[32];
                    gotSomething = true;
                    sprintf(buf,"%04d", index++);
                    tpath = path.replace(idx, 4, buf);
                    if (fdf) {
                        tpath = writeFdfData(gf->getFirstImage(),
                                             tpath.c_str());
                    } else {
                        tpath = writePortableData(gf->getFirstImage(),
                                                  tpath.c_str());
                    }
                }
            }
        }
    } else {
        fprintf(stderr,"No images found\n");
    }
    if (retc > 0) {
        retv[0] = retString(tpath.c_str());
    }
    return proc_complete;
}

string
WriteData::writeFdfData(spImgInfo_t img, const char *filename)
{
    if (img == nullImg) {
	return "";
    }

    // Perhaps append ".fdf" to the filename
    char *newname = (char *)malloc(strlen(filename) + 5);
    const char *tail = filename + strlen(filename) - 4;
    if (strcasecmp(tail, ".fdf") != 0) {
        sprintf(newname,"%s.fdf", filename);
    } else {
        strcpy(newname, filename);
    }

    spDataInfo_t di = img->getDataInfo();
    DDLSymbolTable *cp = (DDLSymbolTable *)di->st->CloneList();
    int outbits = (int)getReal("aipWriteFmtBits", 32);
    if((outbits != 8) && (outbits != 16) && (outbits != 32))
      {
	fprintf(stderr,"Please go to Image->Settings and set Data Output bits\n");
	Werrprintf("%s","Please go to Image->Settings and set Data Output bits\n");	
	free(newname);
	return "";
      }
    
    const char *storage = "integer";
    if (getReal("aipWriteFmtFloat", 1) != 0) {
	storage = "float";
    }

    // Set the scaling parameters
    double max_d = img->getVsMax();
    double min_d = img->getVsMin();
    double vscale;		// Set below for each case
    double voffset = 0;
    int inbits;
    cp->GetValue("bits", inbits);

    cp->SetValue("storage", storage);
    cp->SetValue("bits", outbits);
    cp->SetValue("filename", newname);
    int datawords = cp->DataLength() / (inbits/8);
    if (strcasecmp(storage, "integer") == 0){
	switch (outbits){
	  case 8:
	    {
		unsigned char* dataptr = new unsigned char[datawords];
		vscale = 0xff / (max_d - min_d);
		voffset = - (0xff * min_d) / (max_d - min_d);
		convertData(cp->GetData(),
			    dataptr,
			    datawords,
			    vscale,
			    voffset);
		cp->SetData((float *)dataptr, (outbits/8) * datawords);
		cp->SaveSymbolsAndData(newname);
		delete[] dataptr;
		break;
	    }
	  case 16:
	    {
		unsigned short* dataptr = new unsigned short[datawords];
		vscale = 0x7fff / (max_d - min_d);
		voffset = - (0x7fff * min_d) / (max_d - min_d);
		convertData((float *)cp->GetData(),
			    dataptr,
			    datawords,
			    vscale,
			    voffset);
		cp->SetData((float *)dataptr, (outbits/8) * datawords);
		cp->SaveSymbolsAndData(newname);
		delete[] dataptr;
		break;
	    }
	  case 32:
	    {
		int* dataptr = new int[datawords];
		vscale = 0x7fffffff / (max_d - min_d);
		voffset = - (0x7fffffff * min_d) / (max_d - min_d);
		convertData((float *)cp->GetData(),
			    dataptr,
			    datawords,
			    vscale,
			    voffset);
		cp->SetData((float *)dataptr, (outbits/8) * datawords);
		cp->SaveSymbolsAndData(newname);
		delete[] dataptr;
		break;
	    }
	  default:
	    fprintf(stderr,"Illegal data size for integer FDF data: %d\n",
		    outbits);
	    break;
	}
    } else if (strcasecmp(storage, "float") == 0) {
	switch (outbits) {
	  case 32:
	    {
		cp->SaveSymbolsAndData(newname);
		break;
	    }
	  default:
	    fprintf(stderr,"Illegal data size for float FDF data: %d\n",
		    outbits);
	    break;
	}
    } else {
	fprintf(stderr,"Illegal data type: %s\n", storage);
    }
    cp->Delete();
    return newname;
}

//
//	Write the data displayed in the frame pointed to by "gptr" into
//	the file specified by "path".
//	Data is written in FITS format; integer/float, 8/16/32 bits as
//	specified by the user.  Then the user-specified script is run
//	corresponding to the user specified "Format Type", which (maybe)
//	converts the data into some other format.
//	The "gptr" frame must contain data!
//
/* STATIC */
string
WriteData::writePortableData(spImgInfo_t img, const char *path)
{
    int bits;			// Bits per word
    int bitpix;			// The FITS BITPIX value
    char buf[81];
    const int buflen = sizeof(buf) - 1;
    int i;

    if (img == nullImg) {
	return "";
    }

    // Initialize various parameters
    bitpix = bits = (int)getReal("aipWriteFmtBits", 32);
    if((bits != 8) && (bits != 16) && (bits != 32))
      {
	fprintf(stderr,"Please go to Image->Settings and set Data Output bits\n");
	Werrprintf("%s","Please go to Image->Settings and set Data Output bits\n");	
	return "";
      }
    if (getReal("aipWriteFmtFloat", 1) != 0) {
	bitpix *= -1;
    }
    spDataInfo_t di = img->getDataInfo();
    int nx, ny, nz;
    double len[3];
    nx = di->getFast();
    ny = di->getMedium();
    nz = di->getSlow();
    di->getSpatialSpan(len);
    if (nx == 0 || ny == 0){
	fprintf(stderr,"A spatial dimension is zero.\n");
	return "";
    }

    // Find min and max data values for header
    int datawords = nx * ny;
    float *fdata = di->getData();
    float *edata = fdata + datawords;
    double datamin = *fdata++;
    double datamax = datamin;
    for ( ; fdata<edata; fdata++){
	if (*fdata < datamin){
	    datamin = *fdata;
	}else if (*fdata > datamax){
	    datamax = *fdata;
	}
    }

    // Set the scaling parameters
    double max_d = img->getVsMax();
    double min_d = img->getVsMin();
    double vscale;		// Set below for each case
    double voffset;

    // Convert to the desired data type.
    int databytes =  datawords * bits / 8;
    unsigned char *databuf = new unsigned char[databytes];
    char *pdata;
    short *b16;
    int *b32;
    float *f32;
    switch (bitpix){
      case 8:
	vscale = 0xff / (max_d - min_d);
	voffset = - (0xff * min_d) / (max_d - min_d);
	datamin = datamin * vscale + voffset;
	datamax = datamax * vscale + voffset;
	if (datamin < 0) datamin = 0;
	if (datamax > 0xff) datamax = 0xff;
	convertData(di->getData(), databuf, datawords, vscale, voffset);
	pdata = (char *)databuf;
	break;
      case 16:
	vscale = 0x7fff / (max_d - min_d);
	voffset = - (0x7fff * min_d) / (max_d - min_d);
	datamin = datamin * vscale + voffset;
	datamax = datamax * vscale + voffset;
	if (datamin < -0x7fff) datamin = -0x7fff;
	if (datamax > 0x7fff) datamax = 0x7fff;
	convertData(di->getData(), (short*)databuf, datawords, vscale, voffset);
#ifdef LINUX 
        b16 = (short *) databuf;
        for (int cnt = 0; cnt < datawords; cnt++)
        {
            ByteSwap5(*b16);
            b16++;
        }
#endif
	pdata = (char *)databuf;
	break;
      case 32:
	vscale = 0x7fffffff / (max_d - min_d);
	voffset = - (0x7fffffff * min_d) / (max_d - min_d);
	datamin = datamin * vscale + voffset;
	datamax = datamax * vscale + voffset;
	if (datamin < -0x7fffffff) datamin = -0x7fffffff;
	if (datamax > 0x7fffffff) datamax = 0x7fffffff;
	convertData(di->getData(), (int *)databuf, datawords, vscale, voffset);
#ifdef LINUX 
        b32 = (int *) databuf;
        for (int cnt = 0; cnt < datawords; cnt++)
        {
            ByteSwap5(*b32);
            b32++;
        }
#endif
	pdata = (char *)databuf;
	break;
      case -32:
#ifdef LINUX 
	f32 = (float *)di->getData();
        for (int cnt = 0; cnt < datawords; cnt++)
        {
            ByteSwap5(*f32);
            f32++;
        }
	pdata = (char *)f32;
#else
	pdata = (char *)di->getData();
#endif
	break;
      default:
	if (bitpix < 0) {
	    fprintf(stderr,"Illegal data type (float) for size (%d)\n", bits);
	} else {
	    fprintf(stderr,"Illegal data type (integer) for size (%d)\n", bits);
	}
	delete[] databuf;
	return "";
    }

    // Write data to tmp file in FITS format
    char *tmppath = new char [strlen(path) + 5];
    sprintf(tmppath,"%s.tmp", path);
    int fd = open(tmppath, O_RDWR | O_CREAT, 0664);

    // Write the header
    i=0;
    sprintf(buf,"%-8s=%21s%50s", "SIMPLE", "T", "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s=%21d%50s", "BITPIX", bitpix, "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s=%21d%50s", "NAXIS", 2, "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s=%21d%50s", "NAXIS1", nx, "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s=%21d%50s", "NAXIS2", ny, "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s= '%-8.8s'%60s", "CTYPE1", "CM", "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s= '%-8.8s'%60s", "CTYPE2", "CM", "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s=%21G%50s", "CDELT1", len[0] / nx, "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s=%21G%50s", "CDELT2", len[1] / ny, "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s=%21G%50s", "DATAMIN", datamin, "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s=%21G%50s", "DATAMAX", datamax, "");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-8s%-72s", "HISTORY", "Created by VnmrJ");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%-80s", "END");
    write(fd, buf, buflen); i++;
    sprintf(buf,"%80s", "");
    for ( ; i<36; i++){
	write(fd, buf, buflen);
    }

    // Write the data
    write(fd, pdata, databytes);
    delete[] databuf;

    // Pad out the file to multiple of 2880 bytes
    int npad = 2880 - (databytes % 2880);
    if (npad != 2880){
	char *padbuf = new char[npad];
	memset(padbuf, 0, npad);
	write(fd, padbuf, npad);
	delete[] padbuf;
    }

    close(fd);

    char outpath[1024];
    system(getScript(tmppath, path, outpath));
    unlink(tmppath);
    delete[] tmppath;
    return outpath;
}

//
//	Get the script for converting from the standard output format
//	to the desired format.  Reads the
//	"aipWriteFormatConversionScript" and substitutes the specified
//	strings pointed to by "infile" and "outfile" for the tokens
//	"$1" and "$2", respectively.  ("$x" is converted into "x" if
//	"x" is neither "1" nor "2".)  Returns a string pointer to the
//	appropriate "sh" command(s).
//
const char *
WriteData::getScript(const char *infile, const char *outfile, char *outname)
{
    static char buf[2048];
    const char *end;
    const char *start;
    char tmp[MAXSTR];

    const char *tail = strrchr(outfile, '.');
    int tlen = 0;
    if (tail) {
        tlen = strlen(tail);
    }
    
    *buf = '\0';
    strcpy(tmp,getString("aipWriteFmtConvert", "FITS mv $1 $2").c_str());
    start = tmp;
    while (*start++ != ' ');	// Skip first word
    while ( (end = strchr(start, '$')) ) {
	strncat(buf, start, end-start);
	end++;
	if (*end == '1'){
	    strcat(buf, infile);
	}else if (*end == '2'){
	    strcat(buf, outfile);
            if (tail && strncasecmp(tail, end+1, tlen) == 0) {
                // Already have file extension; skip over extension in script
                end += tlen;
            }
            if (outname) {
                strcpy(outname, outfile);
                for (const char *pc=end+1; isalnum(*pc) || *pc == '.'; ++pc) {
                    strncat(outname, pc, 1);
                }
            }
	}else{
	    strncat(buf, end, 1);
	}
	start = end + 1;
    }
    strcat(buf, start);
    return buf;
}
