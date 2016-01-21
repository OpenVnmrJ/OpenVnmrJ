/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.util.*;
import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import vnmr.templates.*;
import vnmr.util.*;

// Accelerator keys are defined by INTERFACE/acceleratorKeyTable.xml.
// They maybe organized under submenus.
// For convenience, labels, keys and cmds are kept in Hashtables 
// label2key and key2cmd.
// 2D Vector value2D is for making a JTreeModel for AcceleratorKeyEditor. 
// label2key, key2cmd and value2D are updated when the menu tree changed.

// inputKeys are queues for key strokes.
// the queues will be cleared if matched to an existing accelerator keys, or 
// the timer is expired. Esc key will also clear the queues.
// numerical keys are interpreted as one number if interval of entering the keys
// is smaller enough (timed by gtimer).
 
public class AcceleratorKeyTable extends Template
{
    private static AcceleratorKeyTable acceleratorKeyTable = null;
    private Vector value2D = null;
    private Vector header = null;
    private Hashtable label2key = null;
    private Hashtable key2cmd = null;
    public static String KEYS = "keys";
    public static String CMD = "cmd";
    public static String LABEL = "label";
    public static String SUBMENU = "submenu";
    public static String MCHOICE = "mchoice";
    public static String QDELAY = "qdelay";
    public static String KDELAY = "kdelay";

    String accPath = "INTERFACE/acceleratorKeyTable.xml";
    VElement root = null;
    VElement parentNode = null;

    String inputKeys = ""; 
    private javax.swing.Timer timer; //timer for keystrokes in queue (inputKeys) to expire.
    private javax.swing.Timer gtimer; //timer between keystrokes when entering a number. 
    public final static double ONE_SECOND = 1000.0;
    double sec = 2.0; //time before key queue expired.
    double gsec = 0.5; //time allowed between keystokes when entering a number.
    double tm = 0.0;

    Vector menuList = new Vector();

    public AcceleratorKeyTable() {
	initMenu();
	initHeader();

	String delay = "";
	// set queue timer
	if(root != null) delay = root.getAttribute(QDELAY);
	if(delay.length() > 0)
	  tm = Double.valueOf(delay).doubleValue();
	else
 	  tm = ONE_SECOND*sec;
        timer = new javax.swing.Timer((int)tm, new TimerListener());	

	// set key timer
	if(root != null) delay = root.getAttribute(KDELAY);
	else delay ="";
	if(delay.length() > 0)
	  tm = Double.valueOf(delay).doubleValue();
	else 
	  tm = ONE_SECOND*gsec;
        gtimer = new javax.swing.Timer((int)tm, new GTimerListener());	
    }

    public int getQueueDelay() {
	return timer.getDelay();
    }

    public int getKeyDelay() {
	return gtimer.getDelay();
    }

    public void setQueueDelay(double seconds) {
	tm = ONE_SECOND*seconds;
	//timer.setDelay((int)tm);
	timer = new javax.swing.Timer((int)tm, new TimerListener());
	root.setAttribute(QDELAY,String.valueOf(tm));
    }

    public void setKeyDelay(double seconds) {
	tm = ONE_SECOND*seconds;
	//gtimer.setDelay((int)tm);
	gtimer = new javax.swing.Timer((int)tm, new GTimerListener());
	root.setAttribute(KDELAY,String.valueOf(tm));
    }

    public static AcceleratorKeyTable getAcceleratorKeyTable() {
	if(acceleratorKeyTable == null)
	   acceleratorKeyTable = new AcceleratorKeyTable();

	return acceleratorKeyTable;  
    }

    public Hashtable getLabel2keyTable() {
	return label2key;
    }

    public Hashtable getKey2cmdTable() {
	return key2cmd;
    }

    public DefaultTableModel getTableModel() {
	return new MyTableModel(value2D, header);
    }

    protected void setDefaultKeys(){
        setKey(SUBMENU,  vnmr.templates.VElement.class);
        setKey(MCHOICE,  vnmr.templates.VElement.class);
        setKey("template",  vnmr.templates.VElement.class);
        setKey("ref",       vnmr.templates.Reference.class);
        setKey("*Element",  vnmr.templates.VElement.class);
    }

    private void initHeader() {
	header = new Vector();
	header.add(LABEL);
	header.add(KEYS);
	header.add(CMD);
    }

    private void initMenu() {
        label2key = new Hashtable();
        key2cmd = new Hashtable();
	value2D = new Vector();
	String path = FileUtil.openPath(accPath);
	if(path == null) return;

	try {
            open(path);
        }
        catch(Exception e) {
           Messages.postError("could not open study "+path);
           return;
        }
	root=rootElement();
	label2key.clear();
	key2cmd.clear();
	value2D.clear();
	updateTables(root);
/*
	// test
	getMenuList();
	for(int i=0; i<menuList.size(); i++)
	System.out.println("menu name "+menuList.elementAt(i));
*/
    }

    public void saveMenu() {
	// called by AcceleratorKeyEditor when menu (acc key table) changed.
	if(root == null) return;
	String path = FileUtil.savePath(accPath);
	if(path == null) return;
	save(path);
    }

    private void updateTables(VElement elem) {
	if(elem == null) return;
	// called by addKeys and removeKeys to update hashtables when menu tree changed
	VElement child;
        String keys = "";
        String label = "";
        String cmd = "";
	for(int i=0; i< elem.getChildCount(); i++) {
	    child = (VElement)elem.getChildAt(i); 
	    keys = child.getAttribute(KEYS).toUpperCase();
	    label = child.getAttribute(LABEL);
	    cmd = child.getAttribute(CMD);
	    if(child.getNodeName().equals(MCHOICE) &&
		keys.length()>0 && label.length()>0 && cmd.length()>0) {
		label2key.put(label,keys);
		key2cmd.put(keys,cmd);
		Vector row = new Vector();
		row.add(label.trim());
		row.add(keys.trim().toLowerCase());
		row.add(cmd.trim());
		value2D.add(row);
	    } else if(child.hasChildNodes()) updateTables(child);
	}
    }

    public Vector getMenuList() {
	if(root == null) return null;
	// will be called by AcceleratorKeyEditor to make a comboBox menu for
	// possible selection of submenu.
	menuList.clear();
	updateMenuList(root);
	return menuList;
    }

    private void updateMenuList(VElement elem) {
	if(elem == null) return;
	VElement child = null;
	for(int i=0; i< elem.getChildCount(); i++) {
	    child = (VElement)elem.getChildAt(i); 
	    if(child.getNodeName().equals(SUBMENU)) {
		menuList.add(child.getAttribute(LABEL));
	    } else if(child.hasChildNodes()) updateMenuList(child);
	}
	
    }

    public void addKeys(String label, String keys, String cmd, String submenu) {
	// called by AcceleratorKeyEditor to add keys to the menu ans tables
	if(root == null) {
	   newDocument();
	   root = rootElement();
	}
	keys = keys.toUpperCase();
	//System.out.println("addKeys keys "+keys);
	removeKeys(root, keys);
	if(submenu.length() == 0) parentNode = root;
	else {
	  parentNode = null;
	  getParentOfName(root, submenu);
	}
	if(parentNode == null) parentNode = newParent(submenu);
	VElement child = newChild(label, keys, cmd);
	parentNode.appendChild(child);
	label2key.clear();
	key2cmd.clear();
	value2D.clear();
	updateTables(root);
    }

    public void removeKeys(String keys) {
	// called by AcceleratorKeyEditor to remove keys from the menu and tables
	if(root == null) return;
	keys = keys.toUpperCase();
	//System.out.println("removeKeys keys "+keys);
	removeKeys(root, keys);
	label2key.clear();
	key2cmd.clear();
	value2D.clear();
	updateTables(root);
    }

    private VElement newChild(String label, String keys, String cmd) {
	VElement child = newElement(MCHOICE);
	child.setAttribute(LABEL,label);
	child.setAttribute(KEYS,keys);
	child.setAttribute(CMD,cmd);
	return child;
    }

    private VElement newParent(String submenu) {

	VElement newNode = newElement(SUBMENU);
	newNode.setAttribute(LABEL,submenu); 
	root.appendChild(newNode);
	return newNode;
    }

    public void removeKeys(VElement elem, String keys) {
	//recursively search the tree to remove the keys. 
	if(elem == null) return;
	VElement child;
	String key = "";
	for(int i=0; i< elem.getChildCount(); i++) {
	    child = (VElement)elem.getChildAt(i); 
	    key = child.getAttribute(KEYS);
	    if(key.compareToIgnoreCase(keys) == 0) {
		elem.removeChild(child);
	    } else if(child.hasChildNodes()) removeKeys(child, keys);
	}
	if(elem != root && elem.getChildCount() == 0) {
	   VElement parent = (VElement)elem.getParent(); 
	   parent.removeChild(elem);
	}
    }

    public void getParentOfName(VElement elem, String submenu) {
	if(elem == null) return;
	VElement child;
	String label = "";
	for(int i=0; i< elem.getChildCount(); i++) {
	    child = (VElement)elem.getChildAt(i); 
	    label = child.getAttribute(LABEL);
	    if(label.compareToIgnoreCase(submenu) == 0) {
		parentNode = child;
		return;
	    } else if(child.hasChildNodes()) getParentOfName(child, submenu);
	}
    }

    public void processKey(String key, String action) {
	if(root == null) return;
	if(action.equals("keyReleased")) return;

	//System.out.println("processKey pressed " + key);

	if(key.compareToIgnoreCase("Escape") == 0) {
	    if(timer.isRunning()) timer.stop();
	    inputKeys = "";
	    return;
	} 

	key.toUpperCase();
	// for example Return key is "Enter", and make it [Enter]
	if(key.length() > 1) key = "["+key+"]";

	if(inputKeys.length() == 0) {
	   //start timer
	   if(timer.isRunning()) timer.stop();
	   timer.start();
	}
 
	inputKeys += key;

	// if key is numerical, start gtimer.
	// inputKeys will be evaluated when gtimer is out.
	// if key is unnumerical, evaluate inputKeys now. 
	if(gtimer.isRunning()) gtimer.stop();
	if(isDigits(key)) {
	   gtimer.start();
	} else {
	   exeKeys(inputKeys); 
	}

	return;
    }

    private void exeKeys(String str) {

	// try to match str as is.
	// is no match, replace numerical keys with [N], then match again.

	if(str.length() <= 0) return;

	ArrayList nums = new ArrayList();
	if(!matchKeys(str, nums)) {
	   char[] c = str.toCharArray();
	   String keys = "";
	   String num = "";
	   boolean ch = true;
	   for(int i=0; i<c.length; i++) {
		boolean dgt = isDigit(c[i]);
		String s = String.valueOf(c[i]);
		if(dgt && ch) {
		   keys += "[N]";
		   ch = false; 
		   num += s; 
		} else if(dgt) {
		   ch = false; 
		   num += s; 
		} else if(!dgt && !ch) {
		   if(num.length() > 0) nums.add(num);
		   num = "";
		   keys += s;
		   ch = true;
		} else if(!dgt) {
		   num = "";
		   keys += s;
		   ch = true;
		}
	   }
	   // if ends with num, it needs to be added to nums here.
	   if(num.length() > 0) nums.add(num);

	   matchKeys(keys, nums);
	}
    }

    private boolean matchKeys(String str, ArrayList nums) {
    
	// every [N] in keys corresponds to a [n] is the cmd.
	// [n] need to be replaced by actual number in nums.

	String cmd = (String)key2cmd.get(str.toUpperCase());
	if(cmd != null && cmd.length() > 0) {

	   if(timer.isRunning()) timer.stop();
	   if(gtimer.isRunning()) gtimer.stop();

	   int i = 0;
	   while(cmd.indexOf("[n]") != -1 && i < nums.size()) {
		int k = cmd.indexOf("[n]");
                String newcmd = cmd.substring(0,k) + 
			(String)nums.get(i) +cmd.substring(k+3);
		cmd = newcmd;
		i++;
	   }		
	 
	//System.out.println("sendToVnmr keys, cmd  " + str +" "+cmd);

	   Util.sendToVnmr(cmd);
	   inputKeys = "";
	   return true;
	} 
	return false;
    }

    public void showMenu() {
    }

    public void dismissMenu() {
    }

    class TimerListener implements ActionListener {
        public void actionPerformed(ActionEvent evt) {
	//System.out.println("TimerListener empty inputKeys");
	    if(timer.isRunning()) timer.stop();
	    exeKeys(inputKeys);
	    inputKeys = "";
        }
    }

    class GTimerListener implements ActionListener {
        public void actionPerformed(ActionEvent evt) {
	//System.out.println("GTimerListener empty inputKeys");
	   if(gtimer.isRunning()) gtimer.stop();
	   exeKeys(inputKeys);
	   inputKeys = "";
        }
    }

    public boolean isDigit(char c) {

        try {
           Integer.valueOf(String.valueOf(c));
           return true;
        } catch (NumberFormatException e) {
           return false;
        }
    }

    public boolean isDigits(String str) {

        if(str == null || str.length() == 0) return false;
        try {
           Float.valueOf(str);
           return true;
        } catch (NumberFormatException e) {
           return false;
        }
    }

    public class MyTableModel extends DefaultTableModel {

	public MyTableModel(Vector v2d, Vector header) {
	    super(v2d, header);
	}

	public boolean isCellEditable(int row, int col) {
            return false;
        }
    }
}
