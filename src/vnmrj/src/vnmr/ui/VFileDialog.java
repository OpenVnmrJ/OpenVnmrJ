/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import vnmr.util.*;

public class VFileDialog extends FileDialog {

	private String fullPath;
	private String fileName;
	private String directory;
	private int mode;
	
	public VFileDialog(Frame parent, int mode) {
		super(parent, "VnmrJ", mode);
		this.mode = mode;
	}
	
	public void setVisible(boolean v) {
		// for now, we probably never call this with "false" but
		// just to do something sane we do this ...
		super.setVisible(v);
		if (!v) {
			return;
		}
		fileName = this.getFile();
		directory = this.getDirectory();
		fullPath = directory + fileName;
		if (!getDirectoryExtension(directory).equals("fid")) {
			Util.sendToVnmr("write(\'error\',\'unknown file -- open ignored\')");
		} else {
		    if (mode == FileDialog.LOAD) {
			    openNmrFile();
		    } else if (mode == FileDialog.SAVE) {
			    saveFID();
		    }
		}
		this.dispose();
	}
	
	private void openNmrFile() {
		if (fileName.equals("text")) {
			try {
			    Runtime.getRuntime().exec("notepad " + fullPath);
			} catch (Exception e) {}
		} else if (fileName.equals("fid")) {
		    directory = UtilB.windowsPathToUnix(directory);
		    if (directory.charAt(directory.length() - 1) == '/') {
		    	directory = directory.substring(0, directory.length() - 1);
		    }
		    Util.sendToVnmr("rt (\'" + directory + "\')");
		    Util.sendToVnmr("write(\'line3\',\'fid retrieved\')");
		} else if (fileName.equals("procpar")) {
		    directory = UtilB.windowsPathToUnix(directory);
		    if (directory.charAt(directory.length() - 1) == '/') {
		    	directory = directory.substring(0, directory.length() - 1);
		    }
		    Util.sendToVnmr("rtp (\'" + directory + "\')");
		    Util.sendToVnmr("write(\'line3\',\'parameters retrieved\')");
		} else {
			Util.sendToVnmr("write(\'error\',\'unknown file -- open ignored\')");
		}
		
		return;
	}
	
	private void saveFID() {
		return;
	}
	
	// gets the last three characters preceding the end of the string
	// *or* the last three characters preceding the final '/' or '\' --
	// if the result is less than three characters, returns an empty
	// string
	String getDirectoryExtension(String path) {
		String returnValue = new String(path);
		int L;
		
		L = returnValue.length();
		
	    if ((returnValue.charAt(L - 1) == '/') ||
	        (returnValue.charAt(L - 1) == '\\')) {
	    	L = L - 1;
	    	returnValue = returnValue.substring(0, L);
	    }
	    
	    if (L < 3) {
	    	returnValue = "";
	    } else {
	    	returnValue = returnValue.substring(L - 3);
	    }
	    
		return returnValue;
	}

	
}
