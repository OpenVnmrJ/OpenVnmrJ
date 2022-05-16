/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPANNO_H
#define ASPANNO_H

#include <vector>
#include "sharedPtr.h"
#include "AspUtil.h"
#include "AspCell.h"
#include "AspDataInfo.h"

typedef enum
{
    ANNO_NONE = 0,
    ANNO_POINT,
    ANNO_LINE,
    ANNO_ARROW,
    ANNO_BOX,
    ANNO_OVAL,
    ANNO_POLYGON,
    ANNO_POLYLINE,
    ANNO_TEXT,
    ANNO_XBAR,
    ANNO_YBAR,
    ANNO_PEAK,
    ANNO_INTEG,
    ANNO_BAND,
    ANNO_BASEPOINT,
} AnnoType_t;

typedef enum { 
        ANN_SHOW_ROI = 0x100,
        ANN_SHOW_LABEL = 0x200,
        ANN_SHOW_LINK = 0x400,
} AnnoDisFlag_t;

class AspAnno
{ friend class AspAnnoList;

protected:

    AnnoType_t created_type;

    int disFlag;

public:

    int npts;
    Dpoint_t *sCoord;
    Dpoint_t *pCoord;

    int color; // default color for lines is SELECT_COLOR;
    int linkColor; // default PEAK_MARK_COLOR
    string thicknessName; // default line thickness name is "ROILine"  
    string labelFontName; // default label font is "GraphText"

    bool mmbind; // bind to mm position 
    bool mmbindY; // bind to vertical mm position 

    Dpoint_t labelLoc; // relative position of label in ppm if dind=true
    int labelX, labelY, labelW,labelH; 

    // customer line color, thickness, font
    string lineColor;
    int lineThickness;
    string fontName;
    string fontStyle;
    string fontColor; 
    int fontSize;

    string label;

    AspAnno();
    AspAnno(char words[MAXWORDNUM][MAXSTR], int nw);
    virtual ~AspAnno();

    virtual void init();

    void setType(AnnoType_t type) {created_type = type;}
    AnnoType_t getType() {
        return created_type;
    }
    AnnoType_t getType(char *str);
    string getName(AnnoType_t type);

    void setDisFlag(int flag) { disFlag = flag;}

    int getFirstLast(double &x1, double &y1, double &x2, double &y2);
    int getFirstPoint(double &x, double &y);
    bool getCenterPoint(double &x, double &y);

    virtual int select(int x, int y);
    int selectHandle(int x, int y);
    int selectLabel(int x, int y);

    virtual string toString();

    virtual void check(spAspCell_t cell);
    virtual void modify(spAspCell_t cell, int x, int y, int prevX, int prevY);
    virtual void modifyTR(spAspCell_t cell, int x, int y, int prevX, int prevY, bool trCase=false);
    virtual void display(spAspCell_t cell, spAspDataInfo_t dataInfo);
    virtual void getLabel(spAspDataInfo_t dataInfo, string &lbl, int &cwd, int &cht);

    int selected;
    int selectedHandle;

    int index;
    int rotate;
    bool roundBox;
    bool fillRoi;
    double transparency;
    int arrows;
    double amp;
    double vol;
    bool doTraces;

    void setProperty(char *name, char *value, spAspCell_t cell=nullAspCell);
    string getProperty(char *name);

    void setFont(int &labelColor);
    void setRoiColor(int &roiColor, int &thick);

    void getStringSize(string str, int &wd, int &ht); 
    void resetProperties();

    void addPoint(spAspCell_t cell, int x, int y);
    void deletePoint(spAspCell_t cell, int x, int y);
    void insertPoint(spAspCell_t cell, int x, int y);
    
    double getX();
    double getY();
    double getW();
    double getH();
    void setX(spAspCell_t cell, double x);
    void setY(spAspCell_t cell, double y);
    void setW(spAspCell_t cell, double w);
    void setH(spAspCell_t cell, double h);
private:

};

#endif /* ASPANNO_H */
