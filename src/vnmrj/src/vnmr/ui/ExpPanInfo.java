/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.util.*;
import javax.swing.*;

public class ExpPanInfo {
    public int id;
    public int viewport;
    public String name;
    public String param;
    public String fname; // file_name of parameter
    public String afname;  // file_name of action button
    public String fpathIn;
    public String afpathIn;
    public String fpathOut;
    public String defaultDir;
    public String layoutDir;
    public String afpathOut;
    public String ptype;
    public boolean isWaiting; // waiting for new file name
    public boolean toUpdate;
    public boolean isCurrent;
    public boolean needNewFile;
    public boolean bActive;
    public long   dateOfFile;
    public long   dateOfAfile;
    public JComponent paramPanel;
    public JComponent actionPanel;
    public Vector<String> paramVector;

    public ExpPanInfo (int id, String n1, String n2, String n3, String n4,String n5) {
	this.id = id;
	this.name = n1.trim();
	this.fname = n2.trim();
	this.afname = n3.trim();
	this.param = n4.trim();
    this.ptype = n5.trim();
	this.isWaiting = false;
	this.viewport = -1;
	this.paramPanel = null;
	this.actionPanel = null;
	this.toUpdate = true;
	this.needNewFile = true;
	this.bActive = true;
	this.isCurrent = false;
	this.dateOfFile = 0;
	this.dateOfAfile = 0;
	this.defaultDir = null;
	this.layoutDir = null;
	setParamVector();
    }

    public ExpPanInfo (int id, int viewport,
		       String n1, String n2, String n3, String n4, String n5) {
	this.id = id;
	this.viewport = viewport;
	this.name = n1.trim();
	this.fname = n2.trim();
	this.afname = n3.trim();
	this.param = n4.trim();
    this.ptype = n5.trim();
	this.paramPanel = null;
	this.actionPanel = null;
	this.isWaiting = false;
	this.toUpdate = true;
	this.needNewFile = true;
	this.isCurrent = false;
	this.dateOfFile = 0;
	this.dateOfAfile = 0;
	this.defaultDir = null;
	this.layoutDir = null;
	setParamVector();
    }

    private void setParamVector() {
	if (paramVector != null)
	    paramVector.clear();
	else
	    paramVector = new Vector<String>(10);
	if (param == null)
	    return;
	StringTokenizer tok = new StringTokenizer(param, " ,\n");
        while (tok.hasMoreTokens()) {
            String p = tok.nextToken().trim();
	    if (p.length() > 0)
		paramVector.add(p);
	}
    }

    public String toString() {
        return (new StringBuffer().append(getClass().getName()).
                     append(" [id=").append(id).
                     append(", name=").append(name).
                     append(", panel=").append(fname).
                     append(", action=").append(afname).
                     append(", type=").append(ptype).
                     append(", layout=").append(layoutDir).
                     append(", default=").append(defaultDir).append("] ").toString());
    }
}

