/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import javax.swing.*;
import javax.swing.event.*;
import java.awt.*; 
import java.io.*; 
import java.util.*;
import java.awt.event.*; 

import vnmr.bo.VColorLookupPanel; 
import vnmr.ui.ExpPanel; 

@SuppressWarnings("serial")
// public class VColorMapFile extends JFrame
public class VColorMapFile extends JPanel
   implements  ActionListener 
{
   public static String MAP_NAME = "image.cmap";
   public static String SCALE_NAME = "image.scale";
   public static String DEFAULT_NAME = "default";
   public static String SIZE_NAME = "size";
   public static String BEGIN_NAME = "begin";
   public static String END_NAME = "end";
   public static String KNOTS_SIZE = "knots_size";
   public static String RED_KNOTS_INDEX = "red_knots_indexes";
   public static String GREEN_KNOTS_INDEX = "green_knots_indexs";
   public static String BLUE_KNOTS_INDEX = "blue_knots_indexs";
   public static String RED_KNOTS_VALUES = "red_knots_values";
   public static String GREEN_KNOTS_VALUES = "green_knots_values";
   public static String BLUE_KNOTS_VALUES = "blue_knots_values";
   public static String TRANSLUCENCY_NAME = "translucency";
   public static String[] SIZE_NAME_LIST = { "64", "48", "32", "16" };
   public static int[] SIZE_VALUE_LIST = { 64, 48, 32, 16 };
   public static Vector<String> mapNameVector = null;
   public static Vector<String> mapPathVector = null;
   private static java.util.List <VColorMapListenerIF> mapListeners = null;
   private String applySelected = "selected";
   private String applyDisplayed = "displayed";
   private String applyAll = "all";
   private String JFUNC = "jFunc(";
   private String mapFile;
   private String mapName;
   private int   tipIndex;
   private int   sysIndex; // the starting index of system names
   private int   sysLast; // the last index of system names
   private String tipStr;
   private DefaultListModel listModel;
   private JList nameList;
   private JLabel message;
   private JPanel ctrlPan;
   private JTextField nameField;
   private JMenu applyMenu;
   private boolean bDebugMode = false;
   private VColorLookupPanel lookupPanel;
   private Font  ft;

   public VColorMapFile()
   {
	// setTitle("Colormap File");
	listModel = new DefaultListModel();
	nameList = new JList(listModel) {
           public String getToolTipText(MouseEvent e) {
               int index = locationToIndex(e.getPoint());
               if (index >= 0) {
                  return getListToolTip(index);
               }
               return null;
           }
        };

        JScrollPane js = new JScrollPane(nameList,
                            JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                            JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
        js.setAlignmentX(LEFT_ALIGNMENT);

        rebuildList();
	JPanel pane = new JPanel();
	pane.setLayout(new GridLayout(3, 1));
	JPanel namePane = new JPanel();
        SimpleHLayout layout = new SimpleHLayout();
        layout.setExtendLast(true);
	namePane.setLayout(layout);
	namePane.add(new JLabel("Name: "));
	nameField = new JTextField("", 32);
        nameField.setFont(nameList.getFont());
	namePane.add(nameField);
	pane.add(namePane);

        message = new JLabel("   ");
	pane.add(message);

        FlowLayout flow = new FlowLayout();
        flow.setHgap(10);
	ctrlPan = new JPanel(flow);
	addButton("Open", this);
	addButton("Save", this);
	addButton("Delete", this);

        ActionListener ml = new ActionListener() {
            public void actionPerformed(ActionEvent ev) {
                menuAction(ev);
            }
        };

        JMenuBar bar = new JMenuBar();
        applyMenu = new JMenu("Apply");
        VButtonBorder border = new VButtonBorder();
        border.setBorderInsets(new Insets(4,8,4,8));
        applyMenu.setBorder(border);

        String str = Util.getLabel("blSelected", "Selected");
        JMenuItem item = new JMenuItem(str);
        item.setActionCommand(applySelected);
        item.addActionListener(ml);
        applyMenu.add(item);

        str = Util.getLabel("blDisplayed", "Displayed");
        item = new JMenuItem(str);
        item.setActionCommand(applyDisplayed);
        item.addActionListener(ml);
        applyMenu.add(item);

        str = Util.getLabel("blAll", "All");
        item = new JMenuItem(str);
        item.setActionCommand(applyAll);
        item.addActionListener(ml);
        applyMenu.add(item); 

        // bar.add(applyMenu);
	ctrlPan.add(bar);

	addButton("Close", this);

	pane.add(ctrlPan);

        setLayout( new BorderLayout());
	add(js, BorderLayout.CENTER);
	add(pane, "South");

	nameField.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent ev) {
                textChange();
            }
        });

	nameList.addListSelectionListener(new ListSelectionListener() {
            public void valueChanged(ListSelectionEvent e) {
                listChange();
            }
        });
   }

   public void addButton(String label, Object obj)
   {
	XVButton but = new XVButton(Util.getLabel("bl"+label, label));
	but.addActionListener((ActionListener) obj);
	ctrlPan.add(but);
   }

   public void textChange()
   {
   }

   public void setDebugMode(boolean b) {
       bDebugMode = b;
   }

   static public void updateListeners() {
       if (mapListeners == null)
          return;
       Vector v = getMapFileList();
       synchronized(mapListeners) {
          Iterator itr = mapListeners.iterator();
          while (itr.hasNext()) {
               VColorMapListenerIF l = (VColorMapListenerIF)itr.next();
               l.updateMapList(v);
          }
       }
   }

   static public void addListener(VColorMapListenerIF listener) {
       if (mapListeners == null)
           mapListeners = Collections.synchronizedList(new LinkedList<VColorMapListenerIF>());

       if (!mapListeners.contains(listener)) {
           mapListeners.add(listener);
           Vector v = getMapFileList();
           listener.updateMapList(v);
       } 
   }

   static public void removeListener(VColorMapListenerIF listener) {
       mapListeners.remove(listener);
   }

   public static String getUserDir() {
       return FileUtil.usrdir()+FileUtil.separator+"templates"+
                 FileUtil.separator+"colormap";
   }

   public static String getUserMapDir(String name) {
       return getUserDir()+FileUtil.separator+name;
   }

   public static String getUserMapFileName(String name) {
       return getUserMapDir(name)+FileUtil.separator+MAP_NAME;
   }

   public static String getUserScaleFileName(String name) {
       return getUserMapDir(name)+FileUtil.separator+SCALE_NAME;
   }


   public static String getSysDir() {
       return FileUtil.sysdir()+FileUtil.separator+"templates"+
                 FileUtil.separator+"colormap";
   }

   public static String getSysMapDir(String name) {
	return getSysDir()+FileUtil.separator+name;
   }

   public static String getSysMapFileName(String name) {
	return getSysMapDir(name)+FileUtil.separator+MAP_NAME;
   }

   public static String getSysScaleFileName(String name) {
	return getSysMapDir(name)+FileUtil.separator+SCALE_NAME;
   }

   public static String getMapFilePath(String name) {
        if (name == null)
           return null;
        String path = null;
        File fd = new File(name);
        if (!fd.exists() || !fd.canRead()) {
           path = getUserMapFileName(name);
           fd = new File(path);
           if (!fd.exists() || !fd.canRead()) {
              path = getSysMapFileName(name);
              fd = new File(path);
              if (!fd.exists() || !fd.canRead())
                  path = null;
           }
        }
        else
           path = name;
        return path;
   }

   public static String getScaleFilePath(String name) {
        if (name == null)
           return null;
        String path = getUserScaleFileName(name);
        File fd = new File(path);
        if (!fd.exists() || !fd.canRead()) {
           path = getSysScaleFileName(name);
           fd = new File(path);
        }
        if (!fd.exists() || !fd.canRead())
           path = null;
        return path;
   }

   private static void addToVector(String newName, String path) {
        if (mapNameVector == null)
            mapNameVector = new Vector<String>();
        if (mapPathVector == null)
            mapPathVector = new Vector<String>();
        int index = -1;
        for (int k = 0; k < mapNameVector.size(); k++) {
            String name = (String)mapNameVector.elementAt(k);
            if (newName.equals(name))
                return;
            if (newName.compareTo(name) < 0) {
                index = k;
                break;
            } 
        }
        if (index >= 0) {
            mapNameVector.insertElementAt(newName, index);
            mapPathVector.insertElementAt(path+File.separator+MAP_NAME, index);
        }
        else {
            mapNameVector.add(newName);
            mapPathVector.add(path+File.separator+MAP_NAME);
        }
   }

   private static void addMapVector(String path) {
        if (path == null)
            return;
        File dir = new File(path);
        if (!dir.exists())
            return;
        if (!dir.isDirectory())
            return;
        File  files[] = dir.listFiles();
        for (int i = 0; i < files.length; i++) {
            if (files[i].isDirectory()) {
                String name = files[i].getName();
                if (!name.startsWith(".")) {
                     addToVector(name, files[i].getPath());
                }
            }
        }
   }

   public static Vector getMapFileList() {
        if (mapNameVector == null)
            mapNameVector = new Vector<String>();
        else
            mapNameVector.clear();
        if (mapPathVector == null)
            mapPathVector = new Vector<String>();
        else
            mapPathVector.clear();
        addMapVector(getUserDir());
        addMapVector(getSysDir());
        addToVector(DEFAULT_NAME, DEFAULT_NAME);
        return mapNameVector;
   }

   public static Vector getMapPathList() {
        return mapPathVector;
   }

   public String getTemplateName() {
	String d = nameField.getText().trim();
	if (d.length() > 0)
	    return d;
	else
	    return null;
   }

   public void listChange()
   {
	String newTemp = (String)nameList.getSelectedValue();
	if (newTemp != null)
	{
	    nameField.setText(newTemp);
	}
   }

   private void rebuildList() {
        listModel.removeAllElements();
        tipIndex = -1;
        addDirs(getUserDir());
	sysIndex = listModel.getSize();
        addDirs(getSysDir());
	sysLast = listModel.getSize();
        addToFileList(DEFAULT_NAME);
        updateListeners();
   }

   private String getMapName() {
	String name = nameField.getText().trim();
	String mess;
	if ((name == null) || (name.length() <= 0)) {
	    mess = "Name field is empty. ";
	    JOptionPane.showMessageDialog(this, mess);
	    return null;
        }
	return name;
   }

   private void openColormap() {
	String name = getMapName();
        if (name == null)
            return;
        String path = getMapFilePath(name);
        if (path == null && !name.equals(DEFAULT_NAME)) {
            String mess = " Colormap '"+name+"' does not exist. ";
            JOptionPane.showMessageDialog(this, mess);
            return;
        }
        message.setText("Open "+path);
	ImageColorMapEditor.loadColormap(name, path);
   }

   private void saveColormap() {
	String name = getMapName();

        if (name == null)
            return;
	// String data = getSysMapFileName(name);
	String data = getUserMapFileName(name);
        String path = FileUtil.savePath(data, false);
        if (path == null) {
	    // data = getUserMapFileName(name);
            // path = FileUtil.savePath(data, false);
            if (path == null) {
	        data = "Could not save file '"+data+"'.\n Check permissions of this directory and any existing file.";
                JOptionPane.showMessageDialog(this, data);
                return;
            }
        }
        
	File fd = new File(path);
      	if (fd.exists()) {
	    data = "File '"+fd.getParent()+"' already exists.\n Do you want to replace the existing file?";
            int isOk = JOptionPane.showConfirmDialog(this,
                    data, "Save Colormap",
                    JOptionPane.OK_CANCEL_OPTION);
	    if (isOk != 0)
                return;
	}

        message.setText("Save to "+path);
	ImageColorMapEditor.saveColormap(path);
        rebuildList();
        nameList.setSelectedValue(name, true);
   }

   private void deleteColormap() {
	String name = getMapName();

        if (name == null)
            return;
	String data = getUserMapDir(name);
        String path = FileUtil.openPath(data);
        if (path == null) {
	    data = getSysMapDir(name);
            path = FileUtil.savePath(data, false);
            if (path == null) {
                return;
            }
        }

        File fd = new File(path);
        if (!fd.canWrite()) {
	    data = "Could not delete file '"+path+"'.\n Permissions denied.";
            JOptionPane.showMessageDialog(this, data);
            return;
        }
	data = "File '"+path+"' will be deleted!\n Are you sure you want to delete it?";
        int isOk = JOptionPane.showConfirmDialog(this,
                    data, "Delete Colormap",
                    JOptionPane.OK_CANCEL_OPTION);
	if (isOk != 0)
            return;
        if (fd.isDirectory())
            FileUtil.deleteDir(fd);
        else
            fd.delete();
   }

   public void actionPerformed(ActionEvent  evt)
   {
	String ftext = null;
	File fd;
	String cmd = evt.getActionCommand();
	if (cmd.equals("Open"))
	{
	    openColormap();
	    return;
	}
	if (cmd.equals("Save"))
	{
            saveColormap();
	    return;
	}
	if (cmd.equals("Close"))
	{
	    // this.setVisible(false);
            ImageColorMapEditor.close();
	    return;
	}
	if (cmd.equals("Delete"))
	{
            deleteColormap();
            rebuildList();
	    return;
	}
   }

   private void addToFileList(String name) {
	int n = listModel.getSize();
	for (int k = 0; k < n; k++)
	{
	   String ftext = (String)listModel.elementAt(k);
	   if (ftext.equals(name))
		return;
	}
	listModel.addElement(name);
   }

   private void addDirs(String path) {
        if (path == null)
	    return;
	File dir = new File(path);
	if (!dir.exists())
	    return;
	if (!dir.isDirectory())
	    return;
	File  files[] = dir.listFiles();
	for (int i = 0; i < files.length; i++) {
	    if (files[i].isDirectory()) {
		String name = files[i].getName();
		if (!name.startsWith(".")) {
		     addToFileList(name);
		}
	    }
	}
   }

   private String getListToolTip(int n) {
        if (n < 0)
            return null;

        if (tipIndex == n)
            return tipStr;
        String name = (String)listModel.elementAt(n);
        tipIndex = n;
        if (n < sysIndex || n >= sysLast)
            tipStr = getUserDir()+FileUtil.separator+name; 
        else
            tipStr = getSysDir()+FileUtil.separator+name; 
        return tipStr;
   }

   public void setColormapName(String name, String path) {
        mapName = name;
        mapFile = path;
   }   

   public void setLookupPanel(VColorLookupPanel pan) {
        lookupPanel = pan;
   }

   private void menuAction( ActionEvent e )
   {
        String cmd = e.getActionCommand();
        if (bDebugMode)
             System.out.println("apply: "+cmd);
        if (lookupPanel == null)
            return;

        ImageColorMapEditor editor = ImageColorMapEditor.getInstance();

        boolean bNewColor = lookupPanel.isColorChanged();
        String name = mapName;
        File fd = null;
        if (bNewColor) {
            lookupPanel.backupColors();
            name = null;
        }
        if (bDebugMode) {
           System.out.println("color name: "+mapName);
           System.out.println("color changed: "+bNewColor);
        }
        if (name == null) {
            try {
                 fd = File.createTempFile("image", ".cmap");
            }
            catch (IOException er) { return; }

            mapName = fd.getPath();
            fd.deleteOnExit();
            editor.saveColormap(mapName);
        }
        ExpPanel exp = Util.getActiveView();
        if (exp == null)
            return;
        int code = 0; // apply individual
        if (cmd.equals(applyDisplayed))
            code = 1;
        if (cmd.equals(applyAll))
            code = 2;
        if (bDebugMode) {
           System.out.println("apply color name: "+mapName);
        }
        int aipId = 0;
        name = UtilB.windowsPathToUnix(mapName);
        String mess = new StringBuffer().append(JFUNC).append(VnmrKey.APPLYCMP)
                    .append(", ").append(code).append(", ").append(aipId)
                    .append(", '").append(name).append("')\n").toString();
        if (bDebugMode)
           System.out.println("apply cmd: "+mess);
        exp.sendToVnmr(mess);
    }

}


