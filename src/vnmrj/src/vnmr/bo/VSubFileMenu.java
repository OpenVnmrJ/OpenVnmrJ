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
import java.io.*;
import java.util.*;
import javax.swing.*;
import vnmr.util.*;
import vnmr.ui.*;
import vnmr.templates.*;

public class VSubFileMenu extends VSubMenu
{
    private String fName = null;
    private String fPath = null;
    private long   dateOfFile = 0;
    
    public static final String INTERFACE = "INTERFACE/";

    public VSubFileMenu(SessionShare ss, ButtonIF vif, String typ) {
	super(ss, vif, typ);
    }


    public String getAttribute(int attr) {
        switch (attr) {
         case PANEL_FILE:
            return fName;
         default:
            return super.getAttribute(attr);
        }
    }

    public void setAttribute(int attr, String c) {
        switch (attr) {
           case PANEL_FILE:
             fName = c;
             if (!fName.startsWith(INTERFACE))
                  fName = INTERFACE + fName;
             break;
          default:
             super.setAttribute(attr, c);
             break;

        }
    }


    public void updateMe() {
        if (vnmrIf == null)
            return;
	toUpdate = false;
        if (fName != null)
            buildMenu();
    }


    public void buildMenu()
    {
        String strPath = FileUtil.openPath(fName);
        if (strPath == null)
        {
            return;
        }
        
        File fd = new File(strPath);
        if (fd == null || (!fd.exists()))
            return;
        if (strPath.equals(fPath)) {
            if (dateOfFile == fd.lastModified())
                return;
        }
        dateOfFile = fd.lastModified();
        fPath = strPath;
        removeAll();
        if (fPath.endsWith("xml")) {
            try {
                LayoutBuilder.build(this, vnmrIf, fPath);
            }  catch(Exception e) {
                Messages.writeStackTrace(e);
            }
        }
	String att=getAttribute(EXPANDED);
	if (att != null && att.equals("yes"))
	{
           updateChild = true;
           int m = getMenuComponentCount();
           for (int k = 0; k < m; k++) {
               Component comp = getMenuComponent(k);
               if (comp instanceof VMenuitemIF) {
                   VMenuitemIF xobj = (VMenuitemIF) comp;
                   xobj.initItem();
               }
               else if (comp instanceof ExpListenerIF) {
                   ExpListenerIF obj = (ExpListenerIF) comp;
                   obj.updateValue();
               }
           }
	}
    }
}

