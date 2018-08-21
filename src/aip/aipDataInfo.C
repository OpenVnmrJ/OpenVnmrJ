/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cmath>

using namespace std;

#include <string.h>

#include "aipStderr.h"
#include "aipVnmrFuncs.h"
#include "aipUtils.h"
#include "aipDataInfo.h"

#include "aipLine.h"
#include "aipBox.h"
#include "aipOval.h"
#include "aipPolygon.h"
#include "aipPolyline.h"

spDataInfo_t nullData = spDataInfo_t(NULL);

unsigned long DataInfo::nextIndex = 0;

extern "C" {
    int tensor2eulerView(float* theta, float* psi, float* phi, float* orientation);
}

/* STATIC */
/**
 * Construct a useful "key" to be used to sort and retrieve image data.
 * The key uses the "filename" and "display_order" (if present) entries
 * in the FDF header.
 * If both are present, the key is the full path to the file directory
 * followed by a space and the display order in six digits with leading
 * zeros.
 * If "display_order" is not present, the file name is substituted for
 * the display order.
 * If "filename" is not present (should not happen), the key is
 * "No File Name" and is (probably) not unique.
 *
 * Note: Assumes that all files within a directory have unique
 * "display_order", to make unique keys.  If a key is identical to a
 * previously used key, new data will replace the previously loaded
 * data.
 */
string DataInfo::getKey(DDLSymbolTable *st) {
	string key = "";
	if (st != NULL) {
		char *pc;
		string path;
		int order;
		string file = "";
		char strOrder[32];

		//ignore display_order
		//if (!st->GetValue("display_order", order)) {
		order = -1; /* "Unknown" */
		//}
		if (!st->GetValue("filename", pc)) {
			path = string("No File Name");
		} else {
			path = string(pc);
		}

		int islash = path.rfind('/');
		if (islash != string::npos) {
			file = path.substr(islash + 1);
			path = path.substr(0, islash);
		}
		char numCopies[32];
		sprintf(numCopies, " %d", st->GetCopyNumber());
		if (order < 0) {
			key = path + " "+ file + numCopies;
		} else {
			sprintf(strOrder, " %06d", order);
			//key = path + " " + ssBuf.str();
			key = path + strOrder + numCopies;
		}
	}
	return key;
}

string DataInfo::getNameKey(DDLSymbolTable *st) {
	string key = "";
	if (st != NULL) {
		char *pc;
		string path;
		string file = "";

		if (!st->GetValue("filename", pc)) {
			path = string("No File Name");
		} else {
			path = string(pc);
		}

		int islash = path.rfind('/');
		if (islash != string::npos) {
			file = path.substr(islash + 1);
			path = path.substr(0, islash);
		}

		char numCopies[32];
		sprintf(numCopies, " %d", st->GetCopyNumber());
		key = path + " "+ file + numCopies;
	}
	return key;
}

DataInfo::DataInfo(dataStruct_t *data, DDLSymbolTable *newSt,
		DDLSymbolTable *newSt2) {
	aipDprint(DEBUGBIT_0,"DataInfo()\n");

	st = newSt;
	st2 = newSt2;
	dataStruct = data;
	index = nextIndex++;
	relativeIndex = 0; // Mark unknown
	//ostringstream ssBuf;
	//ssBuf << std::setw(8) << std::setfill('0') << dataStruct->order;

	/*char strOrder[32];
	 string path = dataStruct->filepath;
	 string file = "";
	 int islash = path.rfind('/');
	 if (islash != string::npos) {
	 file = path.substr(islash + 1);
	 path = path.substr(0, islash);
	 }

	 char numCopies[32];
	 sprintf(numCopies," %d", newSt->GetCopyNumber());
	 nameKey = path + " " + file + numCopies;
	 if (dataStruct->order < 0) {
	 key = path + " " + file + numCopies;
	 } else {
	 sprintf(strOrder," %06d", dataStruct->order);
	 //key = path + " " + ssBuf.str();
	 key = path + strOrder + numCopies;
	 }
	 */
	key = getKey(newSt);
	nameKey = getNameKey(newSt);
	setGroup();

	long long buflen = dataStruct->matrix[0];
	for (int i = 1; i < dataStruct->rank; i++) {
		buflen *= dataStruct->matrix[i];
	}
	bool phaseImage = (getString("type","absval") != "absval");
	histogram = new Histogram(1024, dataStruct->data, buflen, phaseImage);
	updateScaleFactors();

        shortName="";
        imageNumber="";
	roilist = NULL;
}

DataInfo::~DataInfo() {
	aipDprint(DEBUGBIT_0,"~DataInfo()\n");
	// TODO: Should we use "free" or "delete" here?
	// Probably should use "free", because this structure may have
	// allocated by C code.  But then make sure that C++ always uses
	// malloc for creating any of this stuff.  Maybe better: supply
	// routines for C to call to construct the stuff it needs to.
	if (dataStruct->data) {
		// NB: Only delete the data if the symtab is not going to
		//     delete the same data.
		if (st->GetData() != dataStruct->data) {
			if (isDebugBit(DEBUGBIT_6)) {
				fprintf(stderr,"Deleting struct data at 0x%x\n",
				dataStruct->data);
			}
			delete[] dataStruct->data;
		}
	}
	if (dataStruct->auxparms) {
		char *pc = *(dataStruct->auxparms);
		while (pc) {
			free(pc++);
		}
		free(dataStruct->auxparms);
	}
	delete dataStruct;
	if (st) {
		if (isDebugBit(DEBUGBIT_6)) {
			fprintf(stderr,"Deleting symtab at 0x%x\n", st->GetData());
		}
		st->Delete();
	}
	delete histogram;
}

string DataInfo::getGroup() {
	return group;
}

void DataInfo::setGroup() {
	/*
	 * For now, just set to the directory path.  All images in one
	 * directory are in a group.
	 */
	char *pc;
	if (!st->GetValue("filename", pc)) {
		group = "";
	} else {
		group = string(pc);
		int islash = group.rfind('/');
		if (islash != string::npos) {
			group = group.substr(0, islash);
		}
	}
}

float *DataInfo::getData() {
	if (!dataStruct) {
		return NULL;
	}
	return dataStruct->data;
}

int DataInfo::getFast() {
	if (!dataStruct) {
		return 0;
	}
	return dataStruct->matrix[0];
}

int DataInfo::getMedium() {
	if (!dataStruct) {
		return 0;
	}
	return dataStruct->matrix[1];
}

int DataInfo::getSlow() {
	if (!dataStruct) {
		return 0;
	}
	return dataStruct->matrix[2];
}

double DataInfo::getRatioFast() {
	double ret = 1.0;

	if (st) {
		st->GetValue("span", ret, 0);
	}
	return ret;
}

double DataInfo::getRatioMedium() {
	double ret = 1.0;

	if (st) {
		st->GetValue("span", ret, 1);
	}
	return ret;
}

double DataInfo::getRatioSlow() {
	double ret = 1.0;

	if (st) {
		st->GetValue("span", ret, 2);
	}
	return ret;
}

/*
 * Return the min and max data values.
 */
bool DataInfo::getMinMax(double& dmin, double& dmax) {
	float *data = getData();
	if (!data) {
		return false;
	}

	float *end = data + getFast() * getMedium();
	for (dmin=dmax=*data++; data<end; data++) {
		if (*data > dmax) {
			dmax = *data;
		} else if (*data < dmin) {
			dmin = *data;
		}
	}
	return true;
}

int DataInfo::getEuler(double *euler) {
	if (!dataStruct) {
		return 0;
	}
	for (int i=0; i<3; i++) {
		euler[i] = dataStruct->euler[i];
	}
	return 3;
}

int DataInfo::getOrientation(double *orientation) {
	if (!dataStruct) {
		return 0;
	}
	for (int i=0; i<9; i++) {
		orientation[i] = dataStruct->orientation[i];
	}
	return 9;
}

int DataInfo::getLocation(double *location) {
	if (!dataStruct) {
		return 0;
	}
	for (int i=0; i<3; i++) {
		location[i] = dataStruct->location[i];
	}
	return 3;
}

double DataInfo::getLocation(int i) {
	if (!dataStruct || i > 2) {
		return 0;
	}
	return dataStruct->location[i];
}

int DataInfo::getSpan(double *span, int maxn) {
	if (!dataStruct) {
		return 0;
	}
	int rank = getRank();
	if (rank < maxn) {
		maxn = rank;
	}
	for (int i=0; i<maxn; i++) {
		span[i] = dataStruct->span[i];
	}
	return maxn;
}

double DataInfo::getSpan(int i) {
	if (!dataStruct || i > getRank()-1) {
		return 0;
	}
	return dataStruct->span[i];
}

/*
 * Get the spatial x, y, z size of the data set; i.e., the signed distance
 * in centimeters from the "left" side of the first data point to the
 * "right" side of the last data point.
 * For non-degenerate directions, returns the signed value of "span";
 * for directions that are not "arrayed", returns the unsigned "roi" value.
 * Return Value: number of elements returned
 */
int DataInfo::getSpatialSpan(double *span) {
	if (!dataStruct) {
		return 0;
	}
	int i, j;

	int rank = getRank();
	/*
	 string spatial_rank = getSpatialRank();
	 int sp_rank;	// Number of spatial dims in the data set.
	 if (spatial_rank == "1dfov") {
	 sp_rank = 1;
	 } else if (spatial_rank == "2dfov") {
	 sp_rank = 2;
	 } else if (spatial_rank == "3dfov") {
	 sp_rank = 3;
	 }else if (spatial_rank == "voxel" || spatial_rank == "none") {
	 return 0;
	 }else{
	 fprintf(stderr,"getSpatialSpan: unknown spatial_rank type: \"%s\".",
	 spatial_rank);
	 return 0;
	 }
	 */

	for (i=j=0; i<rank; i++) {
		string s = getAbscissa(i);
		if (s == "cm"|| s == "spatial") {
			span[j++] = getSpan(i);
		}
	}

	// If we do not have 3 spatial spans, fill out array with the
	// corresponding ROI values.
	for (; j<3; j++) {
		span[j] = getRoi(j);
	}

	return 3;
}

string DataInfo::getAbscissa(int i) {
	if (!dataStruct || i > getRank()-1) {
		return 0;
	}
	return dataStruct->abscissa[i];
}

double DataInfo::getRoi(int i) {
	if (!dataStruct || i > 2) {
		return 0;
	}
	return dataStruct->roi[i];
}

int DataInfo::getRank() {
	if (!dataStruct) {
		return 0;
	}
	return dataStruct->rank;
}

const char *DataInfo::getFilepath() {
	char *pc;
	st->GetValue("filename", pc);
	return pc;
}

string DataInfo::getKey() {
	return key;
}

bool DataInfo::updateScaleFactors() {
	int i, j, k;
	double loc[3];
	double orientation[9];
	double span[3];
	double dcos[3][3];
	double wd[3];

	if (getOrientation(orientation) != 9) {
		return false;
	}
	if (getLocation(loc) != 3) {
		return false;
	}
	if (getSpatialSpan(span) != 3) {
		return false;
	}
	span[2] = 0; // Everything is in the slice plane
	wd[0] = getFast();
	wd[1] = getMedium();
	wd[2] = 1;
	for (i=0, j=0; j<3; j++) {
		for (k=0; k<3; k++, i++) {
			dcos[k][j] = orientation[i]; // Load TRANSPOSE of rotation matrix
		}
	}

	// Calculate body-to-magnet rotation 
	calcBodyToMagnetRotation();

	// Calculate data-to-magnet conversion
	for (i=0; i<3; i++) {
		for (j=0; j<2; j++) {
			d2m[i][j] = dcos[i][j] * span[j]/ wd[j];
		}
		d2m[i][2] = 0;
		for (j=0; j<3; j++) {
			d2m[i][2] += dcos[i][j] * (loc[j] - span[j] / 2);
		}
	}

	if (isDebugBit(DEBUGBIT_3)) {
		// Print out matrix
		int i, j;
		fprintf(stderr,"d2m[]:\n");
		for (i=0; i<3; i++) {
			fprintf(stderr,"  {");
			for (j=0; j<3; j++) {
				fprintf(stderr,"%6.3f, ", d2m[i][j]);
			}
			fprintf(stderr,"%6.3f}\n", d2m[i][j]);
		}
	}

	return true;
}

void DataInfo::dataToMagnet(double dx, double dy, double &mx, double &my,
		double &mz) {
	int i, j;
	double d[2];
	double m[3];
	d[0] = dx;
	d[1] = dy;

	for (i=0; i<3; i++) {
		m[i] = d2m[i][2];
		for (j=0; j<2; j++) {
			m[i] += d2m[i][j] * d[j];
		}
	}
	mx = m[0];
	my = m[1];
	mz = m[2];
}

//
// Adjust the positional variables in the FDF header appropriately for
// a 90 deg (counterclockwise) rotation of the image.
// Also adjusts the zooming parameters in the DataInfo header.
// Other DataInfo header parameters (like pixwd) are not basic, and
// are assumed to be recalculated when the image is displayed.
//
void DataInfo::rot90_header() {
	int i;
	double location[3];
	int matrix[2];
	double orient[9];
	double origin[2];
	double roi[3];
	double span[2];
	double zx1;
	double zx2;

	// TODO: Delete these from symbol table instead of setting bogus value.
	st->SetValue("theta", 9999);
	st->SetValue("phi", 9999);
	st->SetValue("psi", 9999);
	GetOrientation(orient);
	get_location(&location[0], &location[1], &location[2]);
	span[0] = getRatioFast();
	span[1] = getRatioMedium();
	for (i=0; i<3; i++) {
		st->GetValue("roi", roi[i], i);
	}
	st->GetValue("origin", origin[0], 0);
	st->GetValue("origin", origin[1], 1);
	matrix[0] = getFast();
	matrix[1] = getMedium();

	st->SetValue("matrix", matrix[1], 0);
	st->SetValue("matrix", matrix[0], 1);
	for (i=0; i<3; i++) {
		st->SetValue("orientation", orient[i+3], i);
	}
	for (i=3; i<6; i++) {
		st->SetValue("orientation", -orient[i-3], i);
	}
	st->SetValue("roi", roi[1], 0);
	st->SetValue("roi", roi[0], 1);
	st->SetValue("origin", origin[1], 0);
	st->SetValue("origin", -(span[0] + origin[0]), 1);
	st->SetValue("span", span[1], 0);
	st->SetValue("span", span[0], 1);
	st->SetValue("location", location[1], 0);
	st->SetValue("location", -location[0], 1);

	//     // Zoom line locations
	//     zx1 = zlinex1;
	//     zx2 = zlinex2;
	//     zlinex1 = zliney1;
	//     zlinex2 = zliney2;
	//     zliney2 = 1 - zx1;
	//     zliney1 = 1 - zx2;

	//     // All the ROItools
	//     RoitoolIterator element(display_list);
	//     Roitool *tool;
	//     while (element.NotEmpty()){
	// 	tool = ++element;
	// 	tool->rot90_data_coords(matrix[0]);
	//     }

	setDataStructureFromSymbolTable(dataStruct, st);
}

void DataInfo::flip_header() {
	int i;
	double location[3];
	int matrix[2];
	double orient[9];
	double origin[2];
	double span[2];

	st->SetValue("theta", 9999);
	st->SetValue("phi", 9999);
	st->SetValue("psi", 9999);
	GetOrientation(orient);
	get_location(&location[0], &location[1], &location[2]);
	span[0] = getRatioFast();
	st->GetValue("origin", origin[0], 0);
	matrix[0] = getFast();

	for (i=0; i<3; i++) {
		st->SetValue("orientation", -orient[i], i);
	}
	st->SetValue("origin", -(span[0] + origin[0]), 0);
	st->SetValue("location", -location[0], 0);

	//     // Zoom line locations
	//     zlinex1 = 1 - zlinex2;
	//     zlinex2 = 1 - zlinex1;

	//     // All the ROItools
	//     RoitoolIterator element(display_list);
	//     Roitool *tool;
	//     while (element.NotEmpty()){
	// 	tool = ++element;
	// 	tool->flip_data_coords(matrix[0]);
	//     }

	setDataStructureFromSymbolTable(dataStruct, st);
}

void DataInfo::GetOrientation(double *orientation) {
	int i;
	for (i=0; i<9; i++) {
		if ( !st->GetValue("orientation", orientation[i], i) ) {
		}
	}
}

// Get the spatial x, y, z location of the data set; i.e., the signed distance
// in centimeters from the center of the magnet to the center of the data set.
// The coordinate system is the "user" coord system--i.e., aligned with the
// data slab.
// Return Value: TRUE on success, FALSE on failure (if the symbol table
//	does not contain the necessary values).


void DataInfo::get_location(double *x, double *y, double *z) {
	if ( !st) {
		STDERR("get_location(): No FDF symbol table.");
	}

	if ( !st->GetValue("location", *x, 0)) {
		STDERR("get_location(): No \"location[0]\" FDF entry.");
	}
	if ( !st->GetValue("location", *y, 1)) {
		STDERR("get_location(): No \"location[1]\" FDF entry.");
	}
	if ( !st->GetValue("location", *z, 2)) {
		STDERR("get_location(): No \"location[2]\" FDF entry.");
	}
}

void DataInfo::initializeSymTab(int new_rank, int new_bit, char * new_type,
		int new_fast, int new_medium, int new_slow, int new_hyperslow) {

	st->SetValue("rank", new_rank);
	st->SetValue("bits", new_bit);
	st->SetValue("storage", new_type);

	st->CreateArray("matrix");
	st->SetValue("matrix", new_fast, 0);
	st->SetValue("matrix", new_medium, 1);
	//     datastx = datasty = 0;
	//     datawd = new_fast;
	//     dataht = new_medium;

	st->CreateArray("abscissa");
	st->SetValue("abscissa", "cm", 0);
	st->SetValue("abscissa", "cm", 1);

	st->CreateArray("ordinate");
	st->SetValue("ordinate", "intensity", 0);

	int typesize; /* data type of size */

	if (strcmp(new_type, "short") == 0)
		typesize = sizeof(short);
	else
		typesize = sizeof(float);

	//   if (alloc_data) {
	//       int filesize = sizeof(Sisheader) +
	// 	  new_fast * new_medium * new_slow * new_hyperslow * typesize;
	//       char *new_data = new char[filesize];
	//       if (new_data == NULL) {
	// 	  PERROR_1("Imginfo(): cannot allocate %d bytes!\n", filesize);
	// 	  return;
	//       }
	//       st->SetData(new_data, filesize);
	//   }

	st->SetValue("spatial_rank", "2dfov");

	/* Don't confuse propar's "type" with ddl's "storage" */
	st->SetValue("type", "absval");

	/* The following values are made up from thin air because
	 * procpar has limited information about them
	 *
	 */

	st->CreateArray("span");
	st->SetValue("span", 10, 0);
	st->SetValue("span", 15, 1);

	st->CreateArray("origin");
	st->SetValue("origin", 5, 0);
	st->SetValue("origin", 7, 1);

	st->CreateArray("nucleus");
	st->SetValue("nucleus", "H1", 0);
	st->SetValue("nucleus", "H1", 1);

	st->CreateArray("nucfreq");
	st->SetValue("nucfreq", 200.067, 0);
	st->SetValue("nucfreq", 200.067, 1);

	st->CreateArray("location");
	st->SetValue("location", 0.0, 0);
	st->SetValue("location", 0.0, 1);
	st->SetValue("location", 0.0, 2);

	st->CreateArray("roi");
	st->SetValue("roi", 10.0, 0);
	st->SetValue("roi", 15.0, 1);
	st->SetValue("roi", 0.2, 2);

	st->CreateArray("orientation");
	st->SetValue("orientation", 1.0, 0);
	st->SetValue("orientation", 0.0, 1);
	st->SetValue("orientation", 0.0, 2);
	st->SetValue("orientation", 0.0, 3);
	st->SetValue("orientation", 1.0, 4);
	st->SetValue("orientation", 0.0, 5);
	st->SetValue("orientation", 0.0, 6);
	st->SetValue("orientation", 0.0, 7);
	st->SetValue("orientation", 1.0, 8);

	/* End of fictional values */
}
bool DataInfo::setDataStructureFromSymbolTable(dataStruct_t *ds,
		DDLSymbolTable *st) {
	char *pc;
	bool ok = true;
	//ignore display_order
	//if (!st->GetValue("display_order", ds->order)) {
	ds->order = -1; /* "Unknown" */
	//}
	if (!st->GetValue("filename", pc)) {
		strcpy(ds->filepath, "No File Name");
	} else {
		strncpy(ds->filepath, pc, sizeof(ds->filepath));
	}
	if (!st->GetValue("type", pc)) {
		ok = false;
		fprintf(stderr,"No \"type\" in FDF header\n");
	} else {
		strncpy(ds->type, pc, sizeof(ds->type));
	}
	if (!st->GetValue("spatial_rank", pc) &&
	!st->GetValue("subrank", pc))
	{
		ok = false;
		fprintf(stderr,"No \"spatial_rank\" in FDF header\n");
	} else {
		strncpy(ds->spatial_rank, pc, sizeof(ds->spatial_rank));
	}
	if (!st->GetValue("storage", pc)) {
		ok = false;
		fprintf(stderr,"No \"storage\" in FDF header\n");
	} else {
		strncpy(ds->storage, pc, sizeof(ds->storage));
	}
	if (!st->GetValue("bits", ds->bits)) {
		ok = false;
		fprintf(stderr,"No \"bits\" in FDF header\n");
	}
	ds->rank = 0;
	if (!st->GetValue("rank", ds->rank)) {
		ok = false;
		fprintf(stderr,"No \"rank\" in FDF header\n");
	}

	if (st->GetValue("ordinate", pc) || st->GetValue("ordinate", pc, 0)) {
		strncpy(ds->ordinate, pc, sizeof(ds->ordinate));
	}
	unsigned long buflen = ds->bits / 8; // Bytes per word
	for (int i=0; i<ds->rank; i++) {
		if (!st->GetValue("matrix", ds->matrix[i], i)) {
			ok = false;
			fprintf(stderr,"No \"matrix\" in FDF header\n");
		}
		buflen *= ds->matrix[i];
		if (!st->GetValue("abscissa", pc, i)) {
			ok = false;
			fprintf(stderr,"No \"abscissa\" in FDF header\n");
		} else {
			strncpy(ds->abscissa[i], pc,
			sizeof(ds->abscissa[i]));
		}
		if (!st->GetValue("span", ds->span[i], i)) {
			ok = false;
			fprintf(stderr,"No \"span\" in FDF header\n");
		}
		if (!st->GetValue("origin", ds->origin[i], i)) {
			ok = false;
			fprintf(stderr,"No \"origin\" in FDF header\n");
		}
	}

	/*
	 * Sanity check on amount of data.
	 */
	if (buflen != st->DataLength()) {
		ok = false;
		char msg[1024];
		sprintf(msg,"Error: Image size expected=%d, Actual buffer size=%d",
		buflen, st->DataLength());
		ib_errmsg(msg);
		return false;
	}

	/*
	 * Get correct Orientation
	 * If "theta", "phi", and "psi" are set, they will have the
	 * correct Euler angles, and "orientation" is likely to be wrong.
	 * That's because if this file came from svib, the orientation
	 * is set from just the first elements of the Euler angle
	 * vectors (if they are arrayed).
	 * Note: svib should now work correctly (as of 2001/07/10).
	 */
	double psi, phi, theta;
	ds->euler[0]=0.0;
	ds->euler[1]=0.0;
	ds->euler[2]=0.0;
	if (ds->rank==2 && strlen(ds->planeOrient) == 0 && st->GetValue("psi", psi) &&
	st->GetValue("phi", phi) &&
	st->GetValue("theta", theta) &&
	psi < 9000 && phi < 9000 && theta < 9000)
	{
		ds->euler[0]=theta;
		ds->euler[1]=psi;
		ds->euler[2]=phi;
		const double deg2rad = M_PI / 180;
		double sintheta = sin(theta * deg2rad);
		double costheta = cos(theta * deg2rad);
		double sinphi = sin(phi * deg2rad);
		double cosphi = cos(phi * deg2rad);
		double sinpsi = sin(psi * deg2rad);
		double cospsi = cos(psi * deg2rad);

		ds->orientation[0] = -cosphi * cospsi - sinphi * costheta * sinpsi;
		ds->orientation[1] = -cosphi * sinpsi + sinphi * costheta * cospsi;
		ds->orientation[2] = -sinphi * sintheta;
		ds->orientation[3] = -sinphi * cospsi + cosphi * costheta * sinpsi;
		ds->orientation[4] = -sinphi * sinpsi - cosphi * costheta * cospsi;
		ds->orientation[5] = cosphi * sintheta;
		ds->orientation[6] = -sintheta * sinpsi;
		ds->orientation[7] = sintheta * cospsi;
		ds->orientation[8] = costheta;

		// Correct the values in the symbol table
		for (int i=0; i<9; i++) {
			st->SetValue("orientation", ds->orientation[i], i);
		}
	} else {
		for (int i=0; i<9; i++) {
			if (!st->GetValue("orientation", ds->orientation[i], i)) {
				ok = false;
				fprintf(stderr,"No \"orientation\" in FDF header\n");
			}
		}
		if(ok) {
		   float dcos[9], eu1,eu2,eu3;
		   for(int i=0;i<9;i++)
			dcos[i]=(float)ds->orientation[i];
		   tensor2eulerView(&eu1, &eu2, &eu3, dcos);
		   ds->euler[0]=eu1;
		   ds->euler[1]=eu2;
		   ds->euler[2]=eu3;
		}
	}

	// TODO: Validate orientation

	if (isDebugBit(DEBUGBIT_3)) {
		// Print out orientation
		if (!st->GetValue("file", pc) && !st->GetValue("filename", pc)) {
			pc = (char *)"Mystery file";
		}
		fprintf(stderr,"Orientation of %s:\n  {", pc);
		for (int i=0; i<9; i++) {
			fprintf(stderr,"%6.3f", ds->orientation[i]);
			if (i == 8) {
				fprintf(stderr,"}\n");
			} else if (i%3 == 2) {
				fprintf(stderr,"}\n  {");
			} else {
				fprintf(stderr,", ");
			}
		}
	}

	for (int i=0; i<3; i++) {
		if (!st->GetValue("location", ds->location[i], i)) {
			ok = false;
			fprintf(stderr,"No \"location\" in FDF header\n");
		}
		if (!st->GetValue("roi", ds->roi[i], i)) {
			ok = false;
			fprintf(stderr,"No \"roi\" in FDF header\n");
		}
	}
	ds->auxparms = NULL; // TODO: Import extra parameters?

	// If data is not float*32, convert it
	if ((strcmp(ds->storage,"float") != 0) || (ds->bits != 32)) {
		if (ds->bits < 8) {
			fprintf(stderr,"Invalid \"bits\" field.");
			return false;
		}
		int datasize = st->DataLength() / (ds->bits/8);
		float* dataptr = new float[datasize];
		if (dataptr == 0) {
			fprintf(stderr,"Unable to allocate memory.");
			return false;
		}

		if (strcmp(ds->storage, "float") == 0) {
			if (ds->bits == 64) {
				convertData( (double *)st->GetData(), dataptr, datasize);
			}
			else {
				fprintf(stderr,"Unsupported bits/storage parameters.");
				return false;
			}
		} else if (strcmp(ds->storage, "char") == 0) {
			if (ds->bits == 8) {
				convertData( (unsigned char *)st->GetData(), dataptr, datasize);
			}
			else {
				fprintf(stderr,"Unsupported bits/storage parameters.");
				return false;
			}
		} else if (strcmp(ds->storage, "short") == 0) {
			if (ds->bits == 16) {
				convertData((unsigned short *)st->GetData(),dataptr,datasize);
			}
			else {
				fprintf(stderr,"Unsupported bits/storage parameters.");
				return false;
			}
		} else if (strncmp(ds->storage, "int", 3) == 0) {
			if (ds->bits == 8) {
				convertData( (unsigned char *)st->GetData(),dataptr,datasize);
			} else if (ds->bits == 16) {
				convertData((unsigned short *)st->GetData(),dataptr,datasize);
			} else if (ds->bits == 32) {
				convertData( (int *)st->GetData(), dataptr, datasize);
			} else {
				fprintf(stderr,"Unsupported bits/storage parameters.");
				return false;
			}
		} else {
			fprintf(stderr,"Unsupported bits/storage parameters.");
			return false;
		}
		st->SetData(dataptr, datasize*(sizeof(float)));
		ds->bits = 32;
		st->SetValue("bits", 32);
		strcpy(ds->storage,"float");
		st->SetValue("storage", "float");
		delete [] dataptr;
	}
	ds->data = st->GetData();

	return ok;
}

bool DataInfo::saveHeader() {
	bool rtn = false;
	string path = getString("filename") + ".aux";
	ofstream hdrfile(path.c_str()); // Note: closes automatically
	if (hdrfile) {
		string topline("#!/usr/local/fdf/startup\n");
		hdrfile.write(topline.c_str(), topline.length());
		if (st2 == NULL)
			return rtn;
		st2->PrintSymbols(hdrfile);
		rtn = true;
	}
	hdrfile.close();
	return rtn;
}

int DataInfo::getInt(const string name, int dflt) {
	int value;
	if (st2 == NULL|| !st2->GetValue(name.c_str(), value)) {
		if (!st->GetValue(name.c_str(), value)) {
			value = dflt;
		}
	}
	return value;
}

int DataInfo::getIntElement(const string name, int idx, int dflt) {
	int value;
	if (st2 == NULL|| !st2->GetValue(name.c_str(), value, idx)) {
		if (!st->GetValue(name.c_str(), value, idx)) {
			value = dflt;
		}
	}
	return value;
}

double DataInfo::getDouble(const string name, double dflt) {
	double value;
	if (st2 == NULL|| !st2->GetValue(name.c_str(), value)) {
		if (!st->GetValue(name.c_str(), value)) {
			value = dflt;
		}
	}
	return value;
}

double DataInfo::getDoubleElement(const string name, int idx, double dflt) {
	double value;
	if (st2 == NULL|| !st2->GetValue(name.c_str(), value, idx)) {
		if (!st->GetValue(name.c_str(), value, idx)) {
			value = dflt;
		}
	}
	return value;
}

string DataInfo::getString(const string name, string dflt) {
	char *pc= NULL;
	string value;
	if (st2 == NULL|| !st2->GetValue(name.c_str(), pc)) {
		if (!st->GetValue(name.c_str(), pc)) {
			value = dflt;
		}
	}
	if (pc) {
		value = string(pc);
	}
	return value;
}

string DataInfo::getStringElement(const string name, int idx, string dflt) {
	char *pc= NULL;
	string value;
	if (st2 == NULL|| !st2->GetValue(name.c_str(), pc, idx)) {
		if (!st->GetValue(name.c_str(), pc, idx)) {
			value = dflt;
		}
	}
	if (pc) {
		value = string(pc);
	}
	return value;
}

bool DataInfo::setInt(const string name, int value) {
	if (st2 == NULL)
		return false;
	return st2->SetValue(name.c_str(), value);
}

bool DataInfo::setDouble(const string name, double value) {
	if (st2 == NULL)
		return false;
	return st2->SetValue(name.c_str(), value);
}

bool DataInfo::setString(const string name, string value) {
	if (st2 == NULL)
		return false;
	return st2->SetValue(name.c_str(), value.c_str());
}

void DataInfo::loadRoi(spGframe_t gf) {
	if (gf == nullFrame || !roilist || roilist->size() <= 0)
		return;
	// make new rois.
	RoiInfoList::iterator itr;
	for (itr = roilist->begin(); itr != roilist->end(); ++itr) {

		if (itr->loaded)
			continue;

		Roi *roi;
		roi = NULL;
		if (itr->name == "Line") {
			roi = new Line(gf, itr->data);
		} else if (itr->name == "Box") {
			roi = new Box(gf, itr->data);
		} else if (itr->name == "Oval") {
			roi = new Oval(gf, itr->data);
		} else if (itr->name == "Polygon") {
			roi = new Polygon(gf, itr->data);
		} else if (itr->name == "Polyline") {
			roi = new Polyline(gf, itr->data);
		}
		itr->loaded = true;
		if (roi != NULL&& itr->selected) {
			roi->select(ROI_NOREFRESH, true);
		}
	}
	roilist->clear();
}

void DataInfo::addRoi(const char *name, spCoordVector_t data, bool loaded,
		bool selected) {
	if (!roilist) {
		roilist = new RoiInfoList;
		if (!roilist)
			return;
	} else {
		RoiInfoList::iterator itr;
		for (itr = roilist->begin(); itr != roilist->end(); ++itr) {
			if (itr->data == data)
				return;
		}
	}

	roiInfo_t info;
	if ( strcmp(name,"") )
		info.name = name;
	else
		info.name = data->name;
	info.data = data;
	info.loaded = loaded;
	info.selected = selected;
	info.active = false;
	roilist->push_back(info);
}

void DataInfo::setRoiSelected_all(bool b) {
	if (!roilist || roilist->size() <= 0)
		return;

	RoiInfoList::iterator itr;
	for (itr = roilist->begin(); itr != roilist->end(); ++itr) {
		itr->selected = b;
	}
}

void DataInfo::setRoiSelected(spCoordVector_t data, bool b) {
	if (!roilist || roilist->size() <= 0)
		return;

	RoiInfoList::iterator itr;
	for (itr = roilist->begin(); itr != roilist->end(); ++itr) {

		if (itr->data == data)
			itr->selected = b;
	}
}

void DataInfo::setRoiActive_all(bool b) {
	if (!roilist || roilist->size() <= 0)
		return;

	RoiInfoList::iterator itr;
	for (itr = roilist->begin(); itr != roilist->end(); ++itr) {
		itr->active = b;
	}
}

void DataInfo::setRoiActive(spCoordVector_t data, bool b) {
	if (!roilist || roilist->size() <= 0)
		return;

	RoiInfoList::iterator itr;
	for (itr = roilist->begin(); itr != roilist->end(); ++itr) {

		if (itr->data == data)
			itr->active = b;
	}
}

void DataInfo::removeAllRois() {
	if (!roilist || roilist->size() <= 0)
		return;
	roilist->clear();
}

void DataInfo::removeSelectedRois() {
	if (!roilist || roilist->size() <= 0)
		return;
	RoiInfoList::iterator itr = roilist->begin();
	while (roilist->size() > 0&& itr != roilist->end()) {
		for (itr = roilist->begin(); itr != roilist->end(); ++itr) {
			if (itr->selected) {
				roilist->erase(itr);
				break;
			}
		}
	}
}

void DataInfo::removeActiveRois() {
	if (!roilist || roilist->size() <= 0)
		return;
	RoiInfoList::iterator itr = roilist->begin();
	while (roilist->size() > 0&& itr != roilist->end()) {
		for (itr = roilist->begin(); itr != roilist->end(); ++itr) {

			if (itr->active) {
				roilist->erase(itr);
				break;
			}
		}
	}
}

void DataInfo::removeRoi(spCoordVector_t data) {
	if (!roilist || roilist->size() <= 0)
		return;
	RoiInfoList::iterator itr;
	for (itr = roilist->begin(); itr != roilist->end(); ++itr) {

		if (itr->data == data) {
			roilist->erase(itr);
			break;
		}
	}
}

void DataInfo::calcBodyToMagnetRotation() {
	char *p1 = (char *)"";
	char *p2 = (char *)"";
	st->GetValue("position1", p1);
	st->GetValue("position2", p2);

	int pos1, pos2;

	if (strcasecmp(p1, "head first") == 0) {
		pos1 = 'h';
	} else if (strcasecmp(p1, "feet first") == 0) {
		pos1 = 'f';
	} else {
		pos1 = 'h'; //return 0;
	}
	if (strcasecmp(p2, "supine") == 0) {
		pos2 = 's';
	} else if (strcasecmp(p2, "prone") == 0) {
		pos2 = 'p';
	} else if (strcasecmp(p2, "left") == 0) {
		pos2 = 'l';
	} else if (strcasecmp(p2, "right") == 0) {
		pos2 = 'r';
	} else {
		pos2 = 's'; //return 0;
	}

	for (int i=0; i<3; ++i) {
		for (int j=0; j<3; ++j) {
			b2m[i][j] = 0;
		}
	}

	if (pos1 == 'f') {
		b2m[2][2] = 1; // Head at +Z
		if (pos2 == 'l') {
			b2m[0][1] = -1; // Nose at -X
		} else if (pos2 == 'r') {
			b2m[0][1] = 1; // Nose at +X
		} else if (pos2 == 'p') {
			b2m[0][0] = -1; // Rt at -X
		} else { /* 's' */
			b2m[0][0] = 1; // Rt at +X
		}
	} else /* 'h' */{
		b2m[2][2] = -1; // Head at -Z
		if (pos2 == 'l') {
			b2m[0][1] = 1; // Nose at +X
		} else if (pos2 == 'r') {
			b2m[0][1] = -1; // Nose at -X
		} else if (pos2 == 'p') {
			b2m[0][0] = 1; // Rt at +X
		} else { /* 's' */
			b2m[0][0] = -1; // Rt at -X
		}
	}
	if (pos2 == 'l') {
		b2m[1][0] = 1; // Rt at +Y
	} else if (pos2 == 'r') {
		b2m[1][0] = -1; // Rt at -Y
	} else if (pos2 == 'p') {
		b2m[1][1] = -1; // Nose at -Y
	} else { /* 's' */
		b2m[1][1] = 1; // Nose at +Y
	}
}
