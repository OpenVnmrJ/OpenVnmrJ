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
import java.awt.event.*;
import java.awt.dnd.*;
import java.awt.datatransfer.*;
import java.io.*;
import javax.swing.*;
import vnmr.util.*;
import vnmr.ui.*;
import vnmr.ui.shuf.*;
import vnmr.templates.*;
import javax.swing.border.Border;

public class VTabPage extends VGroup 
{
    public VTabPage(SessionShare sshare, ButtonIF vif, String typ) {
		super(sshare,vif,typ);
    }
    public String getAttribute(int attr) {
        switch (attr) {
           case TYPE:
               return type;
           case TAB:
               return null;
           default:
               return super.getAttribute(attr);
        }
    }

    public void setAttribute(int attr, String c) {
        switch (attr) {
           case LABEL:
            if(getEditMode()){
        	  Container p=getParent();
        	  if(p instanceof JTabbedPane){
        	  	  JTabbedPane tp=(JTabbedPane)p;
        	  	  int i=tp.indexOfTab(title);
        	  	  if(i>=0)
        	  	  	  tp.setTitleAt(i,c);
        	  }
           }
           title = c;
      	   break;
           default:
        	  super.setAttribute(attr,c);
        }
    }
    public Object[][] getAttributes() {	return attributes; }
    private final static Object[][] attributes = {
    	{new Integer(LABEL),	"Tab label "} 
    };
}

