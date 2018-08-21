/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.awt.*;
import java.util.*;
import java.io.*;
import vnmr.util.*;

public interface  VAnnotationIF {
    public void setEditor(VobjEditorIF editor);
    public void adjustBounds(int row, int col, float xgap, float ygap);
    public void adjustBounds();
    public void setGridInfo(int row, int col, float xgap, float ygap);
    public void readjustBounds(int row, int col, float xgap, float ygap);
    public void readjustBounds();
    public void setBoundsRatio(boolean all);
    public void setRowCol(int rows, int cols);
    public void setXPosition(int x, int y);
    public int getGridNum();
    public void setDropHandler(VDropHandlerIF h);
    public void objPrintf(PrintWriter f, int gap);
    public int getId();
    public void setId(int n);
    public void draw(Graphics2D g, int offsetX, int offsetY);
    public void setPrintMode(boolean b);
    public void setPrintRatio(float w, float h);
    public void setPrintColor(boolean b);
}

