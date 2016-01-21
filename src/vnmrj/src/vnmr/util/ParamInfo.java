/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;
import java.awt.*;
import java.util.*;

import javax.swing.*;

import vnmr.bo.*;
import vnmr.ui.*;

public  class ParamInfo {
	static ParamInfoPanel  infoPanel = null;
    static ParamEditUtil parameditutil=null;

	static Vector  vector = null;
    protected static String m_strhelpfile = "";
    static boolean inEditMode = false;
    
    public static final int  PARAMLAYOUT = 0;
    public static final int  PAGE  = 1;
    public static final int  ACTION  = 2;
    public static final int  ITEM  = 3;
    public static final int  PARAMPANEL  = 4;
    public static final int  TOOLPANEL  = 5;
    
    public static final int PARAMPANELS  =1;
    public static final int ACTIONBAR  =2;
    public static final int TOOLPANELS  =4;
    public static final int STATUSBAR  =8;
   
    static int editItems=PARAMPANELS;
    
    private static ArrayList<EditListenerIF> m_listeners=new ArrayList<EditListenerIF>();
   	private ParamInfo() { }
   	
   	public static void setEditItems(int i){
   	    int olditems=editItems;
   	    editItems=i;
   	    if(olditems!=editItems)
   	        setEditMode(inEditMode);
   	}

    public static void initEditItems(int i){
        editItems=i;
    }

   	public static int getEditItems(){
   	    return editItems;
   	}
    public static void setInfoPanel(String strhelpfile)
    {
        if (infoPanel == null)
            infoPanel = new ParamInfoPanel(strhelpfile);
        m_strhelpfile = strhelpfile;
    }

    public static ParamInfoPanel getInfoPanel()
    {
        return infoPanel;
    }

    public static void clearEditListeners(){
        m_listeners.clear();
    }

    public static void restoreEditMode(){
        setEditMode(inEditMode);
    }

    public static void addEditListener(EditListenerIF e){
        m_listeners.add(e);
    }
    
    public static void setEditMode(boolean b){
        inEditMode=b;
        if(parameditutil==null)
            parameditutil=new ParamEditUtil();
        if (infoPanel == null)
            infoPanel = new ParamInfoPanel(m_strhelpfile);
        infoPanel.setVisible(b);       
        for(int i=0;i<m_listeners.size();i++)
            m_listeners.get(i).setEditMode(b);
    }
	public static void setEditPanel(ParamPanel p,boolean clr) {
	    if (infoPanel == null) {
	       infoPanel = new ParamInfoPanel(m_strhelpfile);
	    }
	    if (p == null)
	       infoPanel.setVisible(false);
	    else if(!infoPanel.isVisible())
	       infoPanel.setVisible(true);
	    infoPanel.setEditPanel(p,clr);
	}

	public static void setEditElement(VObjIF obj) {
	    if (infoPanel == null)
	       infoPanel = new ParamInfoPanel(m_strhelpfile);
	    infoPanel.setEditElement(obj);
	}

	public static void setItemEdited(VObjIF obj) {
	    if (infoPanel == null)
	        infoPanel = new ParamInfoPanel(m_strhelpfile);
	    infoPanel.setItemEdited(obj);
	}
	
	public static void getObjectGeometry() {
	    if (infoPanel == null)
	       infoPanel = new ParamInfoPanel(m_strhelpfile);
	    infoPanel.getObjectGeometry();
	}

//	public static void setUseRef(boolean f) {
//	    if (infoPanel == null)
//	       infoPanel = new ParamInfoPanel(m_strhelpfile);
//	    infoPanel.setUseRef(f);
//	}

	public static void setChangedPage(boolean f) {
	    if (infoPanel == null)
	       infoPanel = new ParamInfoPanel(m_strhelpfile);
	    infoPanel.setChangedPage(f);
	}

	/* no '"' in the string, then call parseStr1, else call parseStr2 */
	public static void parseStr1(String s) {
	    StringTokenizer tok1 = new StringTokenizer(s, " ,\n");
            while (tok1.hasMoreTokens()) {
		String  d = tok1.nextToken().trim();
		if (d.length() > 0)
		    vector.add(d);
	    }
	}

	public static void parseStr2(String s) {
	    String data;
	    int k = s.indexOf('\"');
	    if (k < 0) {
		parseStr1(s);
		return;
	    }
	    if (k > 0) {
		String d = s.substring(0, k).trim();
		if (d.length() > 0)
		    vector.add(d);
		data = s.substring(k+1).trim();
	    }
	    else
		data = s.substring(1).trim();
	    k = data.indexOf('\"');
	    if (k > 0) {
		String s1 = data.substring(0, k);
		parseStr1(s1);
		s1 = data.substring(k+1);
		parseStr2(s1);
	    }
	    else if (k == 0) {
		String s2 = data.substring(1);
		parseStr2(s2);
	    }
	    else
		parseStr1(data);
	}

	public static Vector parseChoiceStr(String str) {
	    if (str == null)
		return null;
	    str = str.trim();
	    if (str.length() < 1)
		return null;
	    vector = new Vector();
/*
	    if (vector == null )
		vector = new Vector();
	    else
		vector.clear();
*/
	    int k = str.indexOf('\"');
	    if (k > 0) {
		String s1 = str.substring(0, k);
		parseStr1(s1);
		s1 = str.substring(k+1);
		parseStr2(s1);
	    }
	    else if (k == 0) {
		String s2 = str.substring(1).trim();
		parseStr2(s2);
	    }
	    else
		parseStr1(str);
	    return vector;
	}
}

