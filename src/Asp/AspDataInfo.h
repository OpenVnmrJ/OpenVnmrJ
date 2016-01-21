/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPDATAINFO_H
#define ASPDATAINFO_H

#include <map>
#include <string>
#include <utility>
#include <list>
using std::list;
#include <set>
using std::set;

#include "sharedPtr.h"
#include "AspUtil.h"

typedef std::map<std::string, aspAxisInfo_t> AxisMap;

class AspDataInfo 
{
public:
	AspDataInfo();
	~AspDataInfo();

	string dataKey;
	aspAxisInfo_t haxis, vaxis;
	int rank;
	bool hasData;

        void updateDataInfo();

        double getVpos();
        double getVoff();
        double getVscale();
        double getHoff();
	string getFidPath();
        int getNtraces(string nucleus);
	void getYminmax(string nucleus, double &ymin, double &ymax);

	void dprint(aspAxisInfo_t *axis);

private:

	void initAxis(int dim, aspAxisInfo_t *axis);
	void init1D_vaxis();
        void initDataInfo();

};

typedef boost::shared_ptr<AspDataInfo> spAspDataInfo_t;
extern spAspDataInfo_t nullAspData;

typedef std::map<std::string, spAspDataInfo_t> AspDataMap;
#endif // ASPDATAINFO_H
