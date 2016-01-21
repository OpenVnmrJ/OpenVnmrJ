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
import java.io.*;
import java.util.*;

import vnmr.util.*;


public class VpLayoutInfo
{
    public int xmainMenu = 0;
    public int xtoolBar = 0;
    public int xsysToolBar = 0;
    public int xgraphicsToolBar = 0;
    public int xinfoBar = 0;
    public int xparam = 0;
    public int xlocator = 0;
    public int vpX = 0;
    public int vpY = 0;
    public int xgrMaxH = 0; // graphMaxHeight
    public int xgrMaxW = 0; // graphMaxWidth
    public int xlocatorTop = 0; // locatorOnTop
    public int xparamTop = 0; // panelOnTop
    public int vpId = 0;
    public float xcontrolX = 0;
    public float xcontrolY = 0;
    public String panelName;
    public String verticalTabName;
    public boolean bAvailable;
    public boolean bNeedSave;

// for VTabbedToolPanel

    final String tp_dividerPrefix = "tp_divider";
    public int tp_selectedTab = -1;
    public Hashtable tp_dividers = null;
 
    public VpLayoutInfo(int num) {
	this.vpId = num;
	this.bAvailable = false;
	this.bNeedSave = false;
	this.panelName = null;
	this.verticalTabName = null;
        readLayoutInfo();
    }

    public void saveLayoutInfo() {
        if (!bNeedSave)
             return;
        String f = Util.checkUiLayout(true, String.valueOf(vpId+1));
        if (f == null)
             return;
        PrintWriter os = null;
        try {
           os = new PrintWriter(new FileWriter(f));
           if (os == null)
                return;
           if (xmainMenu > 0)
                os.println("mainMenu  on");
           else
                os.println("mainMenu  off");
           if (xtoolBar > 0)
                os.println("toolBar  on");
           else
                os.println("toolBar  off");
           if (xinfoBar > 0)
                os.println("infoBar  on");
           else
                os.println("infoBar  off");
           if (xsysToolBar > 0)
                os.println("sysToolBar  on");
           else
                os.println("sysToolBar  off");
           if (xgraphicsToolBar > 0)
                os.println("graphicsToolBar  on");
           else
                os.println("graphicsToolBar  off");
           os.println("paramLocX  "+Float.toString(xcontrolX));
           os.println("paramLocY  "+Float.toString(xcontrolY));
           if (xparam > 0)
                os.println("param  on");
           else
                os.println("param  off");
           if (xlocator > 0)
                os.println("locator  on");
           else
                os.println("locator  off");
           if (xgrMaxH > 0) {
                os.println("graphMaxHeight  yes");
                if (xparamTop > 0)
                    os.println("panelOnTop  yes");
                else
                    os.println("panelOnTop  no");
           }
           else
                os.println("graphMaxHeight  no");
           if (xgrMaxW > 0) {
                os.println("graphMaxWidth  yes");
                if (xlocatorTop > 0)
                    os.println("locatorOnTop  yes");
                else
                    os.println("locatorOnTop  no");
           }
           else
                os.println("graphMaxWidth  no");
           if (panelName != null)
                os.println("panel  "+panelName);
           if (verticalTabName != null)
                os.println("verticalTabName  "+verticalTabName);
	   if(tp_selectedTab >= 0) 
	        os.println("tp_selectedTab " + tp_selectedTab);

	   String key;
	   int value;
	   if(tp_dividers != null && tp_dividers.size() > 0) {
		for(Enumeration e=tp_dividers.keys(); e.hasMoreElements();) {	
		    key = (String)e.nextElement();
		    value = ((Integer)tp_dividers.get(key)).intValue();
	            os.println(tp_dividerPrefix+":"+key +" "+value);
		}
	   }
        }
        catch(IOException er) { }
        os.close();
    }


    public void readLayoutInfo() {
        String  data, atr, v;
        StringTokenizer tok;
        int iv;
        float fv;
        String f = Util.checkUiLayout(false, String.valueOf(vpId+1));

	   if(tp_dividers == null) 
           tp_dividers = new Hashtable();
	   else tp_dividers.clear();

        if (f == null)
             return;
        BufferedReader in = null;
        try {
           in = new BufferedReader(new FileReader(f));
           if (in == null)
                return;

           while ((data = in.readLine()) != null)
           {
                if (data.length() < 2)
                    continue;
                tok = new StringTokenizer(data, " ,\n\t");
                if (!tok.hasMoreTokens())
                    continue;
                atr = tok.nextToken();
                if (!tok.hasMoreTokens())
                    continue;
                data = tok.nextToken();
                if (atr.equals("mainMenu")) {
                    if (data.equals("off"))
                         xmainMenu = 0;
                    else
                         xmainMenu = 1;
                    continue;
                }
                if (atr.equals("paramLocX")) {
                    xcontrolX = Float.parseFloat(data);
                    continue;
                }
                if (atr.equals("paramLocY")) {
                    xcontrolY = Float.parseFloat(data);
                    continue;
                }
                if (atr.equals("toolBar")) {
                    if (data.equals("off"))
                         xtoolBar = 0;
                    else
                         xtoolBar = 1;
                    continue;
                }
                if (atr.equals("infoBar")) {
                    if (data.equals("off"))
                         xinfoBar = 0;
                    else
                         xinfoBar = 1;
                    continue;
                }
                if (atr.equals("param")) {
                    if (data.equals("on"))
                         xparam = 1;
                    else
                         xparam = 0;
                    continue;
                }
                if (atr.equals("locator")) {
                    if (data.equals("on"))
                         xlocator = 1;
                    else
                         xlocator = 0;
                    continue;
                }
                if (atr.equals("graphMaxHeight")) {
                    if (data.equals("yes"))
                         xgrMaxH = 1;
                    else
                         xgrMaxH = 0;
                    continue;
                }
               if (atr.equals("graphMaxWidth")) {
                    if (data.equals("yes"))
                         xgrMaxW = 1;
                    else
                         xgrMaxW = 0;
                    continue;
                }
                if (atr.equals("panelOnTop")) {
                    if (data.equals("yes"))
                         xparamTop = 1;
                    else
                         xparamTop = 0;
                    continue;
                }
                if (atr.equals("locatorOnTop")) {
                    if (data.equals("yes"))
                         xlocatorTop = 1;
                    else
                         xlocatorTop = 0;
                    continue;
                }
                if (atr.equals("panel")) {
                    panelName = data;
                    continue;
                }
                if (atr.equals("verticalTabName")) {
                    verticalTabName = data;
                    continue;
                }
                if (atr.equals("tp_selectedTab")) {
                    tp_selectedTab = Integer.valueOf(data).intValue();
                    continue;
                }
                if (atr.startsWith(tp_dividerPrefix)) {
		    tp_dividers.put(atr.substring(tp_dividerPrefix.length()+1), 
			new Integer(data));
                    continue;
                }
                if (atr.equals("sysToolBar")) {
                    if (data.equals("off"))
                         xsysToolBar = 0;
                    else
                         xsysToolBar = 1;
                    continue;
                }
                if (atr.equals("graphicsToolBar")) {
                    if (data.equals("off"))
                         xgraphicsToolBar = 0;
                    else
                         xgraphicsToolBar = 1;
                    continue;
                }
           } // while
        }
        catch(IOException e) { }
        try {
              in.close();
        } catch(IOException ee) { }

        bAvailable = true;
    }

    public String getPanelName() {
        return panelName;
    }

    public void setPanelName(String name) {
        panelName = name;
    }

    public String getVerticalTabName() {
        return verticalTabName;
    }

    public void setVerticalTabName(String name) {
        verticalTabName = name;
    }
}

