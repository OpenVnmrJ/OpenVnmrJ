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
import java.awt.dnd.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import java.io.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.plaf.*;
import vnmr.util.*;
import vnmr.ui.*;
import vnmr.ui.shuf.*;
import vnmr.templates.*;

public class VShimSet extends VGroup {

    protected String saveKids = "false";
    protected String value = null;
    protected String vnmrVar = null;
    protected String showVal = null;
    protected String setVal = null;
    protected String vnmrCmd = null;
    protected String shimVar = null;
    protected String setShim = null;
    protected String statPar = null;
    protected String fg = null;
    protected String bg = null;
    protected String fontName = null;
    protected String fontStyle = null;
    protected String fontSize = null;
    protected SessionShare sshare;

    // Hard code this for now
/*
    private String path =
	Util.VNMRDIR + File.separator +
	"user_templates" + File.separator +
	"layout" + File.separator +
	"etc" + File.separator +
	"shimbutton.xml";
*/

    private int shimset = -99;
    private int width = 0;
    private int height = 0;
    private Color saveBackground;
    private boolean dropOK;

    public VShimSet(SessionShare sshare, ButtonIF vif, String typ) {
	super(sshare, vif, typ);
	this.sshare = sshare;
        saveBackground = null;
        dropOK = false;
        //setDropTarget(new DropTarget(this, this));
    }

    public boolean getDropOK() {
        return dropOK;
    }

    public void setDropOK(boolean b) {
        dropOK = b;
    }

    public Color getSaveBackground() {
        return saveBackground;
    }

    public void setSaveBackground(Color c) {
        saveBackground = c;
    }

    public String getAttribute(int attr) {
	switch (attr) {
	  case TYPE:
	    return type;
          case VALUE:
	    return value;
	  case SAVEKIDS:
	    return saveKids;
          case SETVAL:
	    return setVal;
          case VARIABLE:
	    return vnmrVar;
          case SHOW:
	    return showVal;
          case CMD:
	    return vnmrCmd;
          case STATPAR:
	    return statPar;
          case VAR2:
	    return shimVar;
          case SETVAL2:
	    return setShim;
	  case FGCOLOR:
	    return fg;
	  case BGCOLOR:
	    return bg;
	  case FONT_NAME:
	    return fontName;
          case FONT_STYLE:
	    return fontStyle;
          case FONT_SIZE:
	    return fontSize;
	  case POINTY:
	  case ROCKER:
	  case ARROW:
	    return VUpDownButton.getStaticAttribute(attr);
	  default:
	    return super.getAttribute(attr);
	}
    }

    public void setAttribute(int attr, String c) {
	switch (attr) {
	  case TYPE:
	    type = c;
          case VALUE:
	    value = c;
	    break;
	  case SAVEKIDS:
	    saveKids = c;
	    autoConfig();
	    break;
          case SETVAL:
	    setVal = c;
	    updateValue();
	    break;
          case VARIABLE:
	    vnmrVar = c;
	    updateValue();
	    break;
          case SHOW:
	    showVal = c;
	    break;
          case CMD:
	    vnmrCmd = c;
	    break;
          case STATPAR:
	    statPar = c;
	    break;
          case VAR2:
	    shimVar = c;
	    break;
          case SETVAL2:
	    setShim = c;
	    break;
	  case FGCOLOR:
	    fg = c;
	    setShimbuttonAttr(attr, c);
	    break;
	  case BGCOLOR:
	    bg = c;
	    setShimbuttonAttr(attr, c);
	    break;
	  case FONT_NAME:
	    fontName = c;
	    setShimbuttonAttr(attr, c);
	    break;
          case FONT_STYLE:
	    fontStyle = c;
	    setShimbuttonAttr(attr, c);
	    break;
          case SETINCREMENTS:
	    setShimbuttonAttr(attr, c);
	    break;
          case POINTY:
          case ROCKER:
          case ARROW:
	    VUpDownButton.setStaticAttribute(attr, c);
	    break;
	  default:
	    super.setAttribute(attr, c);
	}
    }

    public void setValue(ParamIF pf) {
        if (pf != null) {
            value = pf.value;
            autoConfig();
        }
    }

    public void setEditMode(boolean s) {
        super.setEditMode(s);
        autoConfig(true);
        repaint();
    }

    public void setShowValue(ParamIF pf) {
    }

    public void updateValue() {
	if (getVnmrIF() == null)
	    return;
        if (setVal != null) {
	    getVnmrIF().asyncQueryParam(this, setVal);
        }
        if (showVal != null) {
	    getVnmrIF().asyncQueryShow(this, showVal);
        }
    }

    public Object[][] getAttributes() {
	return attributes;
    }

    public void dragOver(DropTargetDragEvent e) {
	if (!e.isDataFlavorSupported(LocalRefSelection.LOCALREF_FLAVOR) &&
            !e.isDataFlavorSupported(DataFlavor.stringFlavor)) {
	    e.rejectDrag();
	}
    }
    public void dragEnter(DropTargetDragEvent e) {
        VObjDropHandler.processDragEnter(e, this, getEditMode());
    }

    public void dragExit(DropTargetEvent e) {
        VObjDropHandler.processDragExit(e, this, getEditMode());
    }

    public void drop(DropTargetDropEvent e) {
        VObjDropHandler.processDrop(e, this, getEditMode());
    }

    private BufferedReader getShimFile(int shimset) {
	String shimfile;
	BufferedReader in;
	shimfile = new String(Util.USERDIR + File.separator + "acqigroups");
	try {
	    // Look for user acqigroup file
	    in = new BufferedReader(new FileReader(shimfile));
	} catch(FileNotFoundException e) {
	    // Use system acqigroupN file
	    shimfile = new String(Util.VNMRDIR + File.separator
				  + "acq" + File.separator
				  + "acqigroup" + shimset);
	    try {
		in = new BufferedReader(new FileReader(shimfile));
	    } catch(FileNotFoundException e2) {
		System.out.println("Cannot open file " + shimfile);
		return null;
	    }
	}
	return in;
    }

    private String readlnSkipComments(BufferedReader in) throws IOException {
	String str;
	do {
	    str = in.readLine();
	    if (str == null) {
		return null;
	    }
	} while (str.length() == 0 || str.charAt(0) == '#');
	return str;
    }

    private ButtonLayout getShimButtonLayout(int shimset) {
	ButtonLayout bLayout = new ButtonLayout();
	ArrayList shimgroups = new ArrayList();
	ArrayList dacinfo = new ArrayList();
	BufferedReader in;
	StringTokenizer toker;
	String str;
	int i;
	int j;

	if ((in = getShimFile(shimset)) == null) {
	    return null;
	}
	try {
	    // Read in old "button groups"
	    while (true) {
		if ((str = readlnSkipComments(in)) == null) {
		    System.out.println("End Of File getting shimgroup label");
		    break;
		}
		if (str.equals("END MANUAL SET")) {
		    break;
		}
		// Have the label, but do not need it
		if ((str = readlnSkipComments(in)) == null) {
		    System.out.println("End Of File getting shim names");
	            in.close();
		    return null;
		}
		shimgroups.add(str); // Append this list of shim names
	    }

	    // Forget the auto groups -- at least for now

	    // Forget the DAC INFO section

	    // Skip to button layout section
	    while (true) {
		if ((str = readlnSkipComments(in)) == null) {
		    //System.out.println("End Of File finding shim layout info");
		    break;
		}
		if (str.equals("BEGIN BUTTON LAYOUT")) {
		    break;
		}
	    }
	    // Read button layout
	    while (true) {
		if ((str = readlnSkipComments(in)) == null) {
		    //System.out.println("End Of File getting shim layout info");
		    break;
		}
		if (str.equals("END BUTTON LAYOUT")) {
		    break;
		}

		toker = new StringTokenizer(str);
		String name;
		ShimButtonSpec bSpec;
		i = 0;
		try {
		    for (i=0; true; i++) {
			name = toker.nextToken();
			if (name.charAt(0) != '-') {
			    bSpec = new ShimButtonSpec(bLayout.nRows, i, name);
			    bLayout.add(bSpec);
			}
		    }
		} catch(NoSuchElementException e) {
		    bLayout.nRows++;
		    if (i > bLayout.nCols) {
			bLayout.nCols = i;
		    }
		}
	    }

	    in.close();
	} catch(IOException e) {
	}

	if (bLayout.size() == 0) {
	    // No layout info--use the shim groups
	    String name;
	    ShimButtonSpec bSpec;
	    int nrows;
	    for (int icol=0; icol<shimgroups.size(); icol++) {
		bLayout.nCols = shimgroups.size();
		toker = new StringTokenizer((String)shimgroups.get(icol));
		nrows = toker.countTokens();
		if (nrows > bLayout.nRows) {
		    bLayout.nRows = nrows;
		}
		int irow;
		for (irow = i = 0; i<nrows; i++) {
		    name = toker.nextToken().toLowerCase();
		    if (bLayout.getEntry(name) == null) {
			// Not a duplicate, put in the info
			bSpec = new ShimButtonSpec(irow, icol, name);
			bLayout.add(bSpec);
		    }
		    irow++;
		}
	    }
	}

	// Put in label, increment info
	String systemType = "spectrometer";
	String prop = "vnmr.properties.ShimLabels";
	ResourceBundle labels = ResourceBundle.getBundle(prop);
	for (i=0; i<bLayout.size(); i++) {
	    ShimButtonSpec bSpec = (ShimButtonSpec)bLayout.get(i);

	    // Get label from properties file
	    bSpec.label = bSpec.name.toUpperCase();// Temporary

	    // Get increment info

	    // Get shim limits from Vnmr

	}

	return bLayout;
    }

    private class ButtonLayout extends ArrayList {
	public int nRows = 0;
	public int nCols = 0;

	ShimButtonSpec getEntry(String name) {
	    ShimButtonSpec sbs;
	    for (int i=0; i<size(); i++) {
		sbs = (ShimButtonSpec)get(i);
		if (name.equals(sbs.name)) {
		    return sbs;
		}
	    }
	    return null;
	}

	ShimButtonSpec getEntry(int row, int col) {
	    ShimButtonSpec sbs;
	    for (int i=0; i<size(); i++) {
		sbs = (ShimButtonSpec)get(i);
		if (sbs.row == row && sbs.column == col) {
		    return sbs;
		}
	    }
	    return null;
	}
    }

    private class ShimButtonSpec {
	String name;
	String label = "";
	int value = 0;
	int min = 0;
	int max = 0;
	int incr1 = 5;
	int incr2 = 20;
	int row;
	int column;

	ShimButtonSpec(int row, int column, String name) {
	    this.row = row;
	    this.column = column;
	    this.name = name;
	}
    }

    private void autoConfig() {
	autoConfig(false);
    }

    private void rebuildSet() {

	Dimension dim = getPreferredSize();
	int gw = dim.width;	// Group width
	int gh = dim.height;	// Group height

	removeAll();

	ButtonLayout bLayout = getShimButtonLayout(shimset);
	Insets insets = new Insets(8, 5, 5, 5);
	int hGap = 2;		// Horizontal gap between buttons
	int vGap = 3;		// Vertical gap
	int offSet = 5;
	int nRows = bLayout.nRows;
	int nCols = bLayout.nCols;
	int bh, bw;		// Button width, height
	int x, y;
	bw = ((dim.width - offSet * 2) - nCols * hGap) / nCols;
	bh = ((dim.height - offSet * 2) - (nRows - 1) * vGap) / nRows;
	ButtonIF vif = getVnmrIF();
	// Put in new buttons
	VUpDownButton vb;
	ShimButtonSpec bSpec;
	for (int irow=0; irow<nRows; irow++) {
	    y = offSet + irow * (vGap + bh);
	    for (int icol=0; icol<nCols; icol++) {
		bSpec = (ShimButtonSpec)bLayout.getEntry(irow, icol);
		if (bSpec != null) {
		    x = offSet + icol * (hGap + bw);
		    vb = new VUpDownButton(sshare, vif, "shimbutton");
		    if(getEditMode())
		        vb.setEnabled(false);
		    else 
		        vb.setEnabled(true);
		   // vb.setLocation(x, y);
		    vb.setDefLoc(x, y);
		    vb.setPreferredSize(new Dimension(bw, bh));
		    vb.setBounds(x, y, bw, bh);
		    vb.setAttribute(LABEL, bSpec.label);
		    vb.setAttribute(VAR2, bSpec.name);
		    vb.setAttribute(STATPAR, nameSub(statPar, bSpec.name));
		    vb.setAttribute(VARIABLE, nameSub(shimVar, bSpec.name));
		    vb.setAttribute(SETVAL, nameSub(setShim, bSpec.name));
		    vb.setAttribute(CMD, nameSub(vnmrCmd, bSpec.name));
		    vb.setAttribute(FONT_NAME, getAttribute(FONT_NAME));
		    vb.setAttribute(FONT_STYLE, getAttribute(FONT_STYLE));
		    vb.setAttribute(FONT_SIZE, getAttribute(FONT_SIZE));
		    vb.changeFont();
		    vb.setAttribute(POINTY, getAttribute(POINTY));
		    vb.setAttribute(ROCKER, getAttribute(ROCKER));
		    vb.setAttribute(ARROW, getAttribute(ARROW));
		    add(vb);
		}
	    }
	}
	validate();
	getActualSize();
    }

    private void autoConfig(boolean force) {
	if (value == null || value.length() == 0) {
	    return;
	}
        /*System.out.println("autoConfig(force="+force+")"
                           +": saveKids="+saveKids
                           );/*CMP*/
	if (!force && (saveKids == null || !saveKids.equals("false")) )	{
	    return;
	}
	int shimset = 1;
	try {
	    shimset = Integer.parseInt(value);
	} catch (NumberFormatException ex) { }
	
	Dimension dim = getSize();
	int gw = dim.width;	// Group width
	int gh = dim.height;	// Group height

	if (shimset != this.shimset)
	    force = true;

	if (!force && this.width == gw && this.height == gh)
	{
	    return;
	}
	this.shimset = shimset;
	if (force) {
	    rebuildSet();
	    resizeAll();
	}

/*
	this.width = gw;
	this.height = gh;

	// Put in new buttons
	ButtonLayout bLayout = getShimButtonLayout(shimset);
	Insets insets = new Insets(8, 5, 5, 5);
	int hGap = 2;		// Horizontal gap between buttons
	int vGap = 3;		// Vertical gap
	int nRows = bLayout.nRows;
	int nCols = bLayout.nCols;
	int bh, bw;		// Button width, height
	bw = ((gw - insets.left - insets.right) - (nCols - 1) * hGap) / nCols;
	bh = ((gh - insets.top - insets.bottom) - (nRows - 1) * vGap) / nRows;
	ButtonIF vif = getVnmrIF();
	VUpDownButton vb;
	ShimButtonSpec bSpec;
	for (int irow=0; irow<nRows; irow++) {
	    for (int icol=0; icol<nCols; icol++) {
		bSpec = (ShimButtonSpec)bLayout.getEntry(irow, icol);
		if (bSpec != null) {
		    int x = insets.left + icol * (hGap + bw);
		    int y = insets.top + irow * (vGap + bh);
		    vb = new VUpDownButton(sshare, vif, "shimbutton");
		    if(getEditMode())
		        vb.setEnabled(false);
		    else 
		        vb.setEnabled(true);
		    vb.setLocation(x, y);
		    vb.setPreferredSize(new Dimension(bw, bh));
		    vb.setBounds(x, y, bw, bh);
		    vb.setAttribute(LABEL, bSpec.label);
		    vb.setAttribute(VAR2, bSpec.name);
		    vb.setAttribute(STATPAR, nameSub(statPar, bSpec.name));
		    vb.setAttribute(VARIABLE, nameSub(shimVar, bSpec.name));
		    vb.setAttribute(SETVAL, nameSub(setShim, bSpec.name));
		    vb.setAttribute(CMD, nameSub(vnmrCmd, bSpec.name));
		    vb.setAttribute(FONT_NAME, getAttribute(FONT_NAME));
		    vb.setAttribute(FONT_STYLE, getAttribute(FONT_STYLE));
		    vb.setAttribute(FONT_SIZE, getAttribute(FONT_SIZE));
		    vb.changeFont();
		    vb.setAttribute(POINTY, getAttribute(POINTY));
		    vb.setAttribute(ROCKER, getAttribute(ROCKER));
		    vb.setAttribute(ARROW, getAttribute(ARROW));
		    add(vb);
		}
	    }
	}
*/
	validate();
	repaint();
    }

    private String nameSub(String str, String name) {
	int i;
	if (str == null || (i = str.lastIndexOf("$NAME")) < 0) {
	    return str;
	}
	StringBuffer rtn = new StringBuffer(str);
	do {
	    rtn.replace(i, i+5, name);
	} while ((i = str.lastIndexOf("$NAME", i-5)) >= 0);
	return rtn.toString();
    }

    private void setShimbuttonAttr(int attr, String c) {
	if (saveKids != null && saveKids.equals("true")) {
	    return;		// Don't control them if frozen
	}
	int n = getComponentCount();
	for (int i=0; i<n; i++) {
	    Component comp = getComponent(i);
	    if (comp instanceof VUpDownButton) {
		VObjIF vcomp = (VObjIF)comp;
		vcomp.setAttribute(attr, c);
		if (attr == FONT_NAME
		    || attr == FONT_STYLE || attr == FONT_SIZE)
		{
		    vcomp.changeFont();
		}
	    }
	}
    }

    public void changeFont() {
	if (saveKids != null && saveKids.equals("true")) {
	    return;		// Don't control them if frozen
	}
	int n = getComponentCount();
	for (int i=0; i<n; i++) {
	    Component comp = getComponent(i);
	    if (comp instanceof VUpDownButton) {
		VObjIF vcomp = (VObjIF)comp;
		vcomp.changeFont();
	    }
	}
    }

    private final static String[] bdrTypes =
    {"None", "Etched", "RaisedBevel", "LoweredBevel"};
    private final static String[] true_false = {"true", "false"};
    private final static Object[][] attributes = {
	{new Integer(BORDER),	"Border type:", "menu", bdrTypes},
	{new Integer(SAVEKIDS),	"Freeze layout:", "menu", true_false},
	{new Integer(VARIABLE),	"Vnmr shim set parameter:"},
	{new Integer(SETVAL),	"Vnmr shim set value:"},
	{new Integer(CMD),	"Shim setting command:"},
	{new Integer(STATPAR),	"Status parameter for shim:"},
	{new Integer(VAR2),	"Vnmr variables for shim:"},
	{new Integer(SETVAL2),	"Vnmr expression for shim:"},
    };
    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}
}

