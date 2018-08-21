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
import javax.swing.text.*;
import javax.swing.border.*;
import java.io.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import vnmr.sms.*;
import javax.swing.plaf.basic.BasicSplitPaneUI;
import javax.swing.plaf.metal.MetalSplitPaneUI;


import vnmr.ui.*;
import vnmr.templates.*;
import vnmr.bo.*;

// public class AnnotationEditor extends ModelessDialog 
public class AnnotationEditor extends JFrame
      implements ActionListener, VObjDef, VobjEditorIF, VnmrKey
{
    private JSplitPane mainPan;
    private JPanel topPan;
    private JPanel objPan;
    private JPanel infoPan;
    private JPanel attrPan;
    private JPanel prefPan;
    private JPanel ctlPan;
    private JPanel varPans[];
    private JLabel varLabels[];
    private JComponent varObjs[];
    private VAnnotatePanel dispPan;

    private VObjIF editVobj = null;
    private JTextField snapSize;
    private JTextField activeEntry = null;
    private JComboBox tempList;
    private JComboBox parList;

    private JButton saveButton=null;
    private JButton reloadButton=null;
    private JButton clearButton=null;

    private JScrollPane attrScroll;

    private JRadioButton snapOn;
    private JRadioButton snapOff;
    private JRadioButton editableOn;
    private JRadioButton editableOff;
    private JCheckBox orientCheck;
    private VInfoObj  templateObj;

    private Vector fList = null;
    private String userPath = null;
    private String sysPath = null;
    private String curTemplate = null;
    private Color gridColor = Color.black;
    private int attrWidth=22;
    private int sbW = 0;
    private int gridNum = 40;
    private int windowWidth = 700;
    private int scrollH = 30;
    private int curAttrCode = 0;

    private Font font = new Font("Dialog", Font.PLAIN, 12);

    private TextChangeListener textListener;
    private SimpleH2Layout xLayout = new SimpleH2Layout(SimpleH2Layout.LEFT,
                                                       4, 0, true, true);
    private SimpleH2Layout tLayout = new SimpleH2Layout(SimpleH2Layout.LEFT,
                                                       4, 0, false, true);

    private JLabel typeOfobj;
    private VAnnChooser textStyle;
    private JLabel parLabel;

    private static boolean debugMode = false;
    private static Color bgColor;
    private boolean bNewParm = true;
    
    public AnnotationEditor() {
         super("Imaging Annotation Editor");
         textListener = new TextChangeListener(); 
         if (debugMode) {
              DisplayOptions disp = new DisplayOptions(null, null);
              disp.build();
              Util.setDisplayOptions(disp);
         }
         userPath = "USER/VNMRJ/annotation/";
         sysPath = "SYSTEM/imaging/templates/vnmrj/annotation/";
/*
         sysPath = "SYSTEM/VNMRJ/annotation/";
*/
         mainPan = new JSplitPane(JSplitPane.VERTICAL_SPLIT);
         mainPan.setUI(new BasicSplitPaneUI()); 
         //  mainPan.setUI(new MetalSplitPaneUI()); 
         if (debugMode) {
             setBackground(Color.lightGray);
             mainPan.setBackground(Color.lightGray);
         }
         else {
             setBackground(Util.getBgColor());
             mainPan.setBackground(Util.getBgColor());
         }
         // mainPan.setOneTouchExpandable(true);
         mainPan.setDividerSize(10);
         createEditPanel();
         buildObjPanel();
         mainPan.setTopComponent(topPan);
         createInfoPanel();
         buildTemplateList();
         mainPan.setBottomComponent(infoPan);
         Container cont = getContentPane();
         cont.add(mainPan, BorderLayout.CENTER);

         pack();
         Toolkit tk = Toolkit.getDefaultToolkit();
         Dimension screenDim = tk.getScreenSize();
         int h = screenDim.height - 60;
         int w = h * 7 / 10;
         setSize(w, h);
         setLocation(30, 0);
         mainPan.setDividerLocation(0.7);
         // initTemplate(null);
         buildParamSource();
         templateObj = new VInfoObj("$VALUE=aipAnnotation", 1);
         templateObj.updateValue();
         validate();
    }


    private void loadTemplate(String f) {
         setEditObj(null);
         dispPan.loadTemplate(f);
         int k = dispPan.getGridNum();
         snapSize.setText(Integer.toString(k));
    }

    private void loadSelectedTemplate() {
         String path, fname;
         String f = getTemplateName();
         if (f == null)
             return;
         fname = userPath + f + ".xml";
         path = FileUtil.openPath(fname);
         if (path == null) {
              fname = sysPath + f + ".xml";
              path = FileUtil.openPath(fname);
         }
         if (path != null) {
              curTemplate = f;
              loadTemplate(path);
         }
    }

    private void initTemplate(String name) {
         int vsize = fList.size();
         int k = -1;
         String path, f;
         String s = name;
         if (s == null || s.length() < 1)
             s = "default"; 
         while (k < vsize) {
             f = userPath+s+".xml";
             path = FileUtil.openPath(f);
             if (path == null) {
                f = sysPath+s+".xml";
                path = FileUtil.openPath(f);
             }
             if (path != null) {
                curTemplate = s;
                loadTemplate(path);
                tempList.setSelectedItem(s);
                break;
             }
             k++;
             if (k >= vsize)
                 break;
             s = (String) fList.elementAt(k);
         }
    }

    public void setVisible(boolean b) {
         if (b) {
             if (bNewParm) 
                buildParamSource();
         }
         else
             bNewParm = true;
         super.setVisible(b);
         if (b && templateObj != null)
             templateObj.updateValue();
    }

    private void createEditPanel() {
         topPan = new JPanel(new BorderLayout());
         objPan = new JPanel();
         dispPan = new VAnnotatePanel(this);
         dispPan.setBorder(new EtchedBorder());
         // objPan.setPreferredSize(new Dimension(60, 800));
         topPan.setBackground(Util.getBgColor());
         topPan.add(objPan, BorderLayout.WEST);
         topPan.add(dispPan, BorderLayout.CENTER);
         dispPan.setEditor(this);

         // topPan.setSize(400, 400);
    }

    private void buildObjPanel() {
         AnnotateObj obj = new AnnotateObj("New\nGroup");
         obj.setToolTipText("Drag and drop to create Group");
         obj.setName("box");
         objPan.add(obj);

         obj = new AnnotateObj("New\nTable");
         obj.setToolTipText("Drag and drop to create table");
         obj.setName("table");
         objPan.add(obj);

         obj = new AnnotateObj("New\nLabel");
         obj.setToolTipText("Drag and drop to create Label");
         obj.setName("label");
         objPan.add(obj);

         obj = new AnnotateObj("New\nTextmessage");
         obj.setToolTipText("Drag and drop to create Text Message");
         obj.setName("text");
         objPan.add(obj);

         orientCheck = new JCheckBox("Show Orientation");
         orientCheck.setActionCommand("orientation");
         orientCheck.addActionListener((ActionListener) this);
         objPan.add(orientCheck);

         JButton but = addButton("Clear Selected", objPan, this, "deleteObj");
         but.setToolTipText("Clear selected item");
         but = addButton("Clear All", objPan, this, "deleteAll");

         but.setToolTipText("Clear all items");
         objPan.setLayout(new SimpleVLayout(6, 10, false));
    }

    private void createInfoPanel() {
         infoPan = new JPanel();

         varPans = new JPanel[20];
         varLabels = new JLabel[20];
         varObjs = new JComponent[20];
       
         JPanel typePan = new JPanel();
         typePan.setLayout(new SimpleH2Layout(SimpleH2Layout.LEFT, 10, 0,false));
         JLabel lb = new JLabel("Type of element: ");
         typePan.add(lb);

         typeOfobj = new JLabel("none");
         typePan.add(typeOfobj);

         parLabel = new JLabel("Parameters:");
         parLabel.setToolTipText("parameter to be inserted into the entry");
         parList = new JComboBox();
         parList.addActionListener(new ParamListener());
         parList.addPopupMenuListener(new PopupMenuListener() {
                public void popupMenuWillBecomeVisible(PopupMenuEvent e) {
                     getActiveEntry();
                }
                public void popupMenuWillBecomeInvisible(PopupMenuEvent e) {
                     setEntryActive();
                }
                public void popupMenuCanceled(PopupMenuEvent e) {
                     setEntryActive();
                }
         });

         JPanel fontPan = new JPanel();
         fontPan.setLayout(new SimpleH2Layout(SimpleH2Layout.LEFT, 10, 0, false));

         lb = new JLabel("Style of element: ");
         fontPan.add(lb);
         textStyle = new VAnnChooser("style");
         fontPan.add(textStyle);
         addButton("Edit Styles", fontPan, this, "editStyles");

         JPanel headerPan = new JPanel();
         headerPan.setLayout(new attrLayout());
         headerPan.setBorder(new EtchedBorder(EtchedBorder.LOWERED));
         headerPan.add(typePan);
         headerPan.add(fontPan);

         fontPan.add(parLabel);
         fontPan.add(parList);

         attrPan=new JPanel();
         attrPan.setBorder(new BevelBorder(BevelBorder.LOWERED));
         attrPan.setLayout(new attrLayout());
         attrScroll = new JScrollPane(attrPan);
         attrScroll.setPreferredSize(new Dimension(600, 200));

         prefPan = new JPanel();
         prefPan.setLayout(new SimpleH2Layout(SimpleH2Layout.LEFT, 30, 0, false));
         prefPan.setBorder(new EtchedBorder(EtchedBorder.LOWERED));

         JPanel snapPan = new JPanel();
         snapPan.setLayout(new SimpleH2Layout(SimpleH2Layout.LEFT, 5, 0, false));
         lb = new JLabel("Grid Number");
         snapSize = new JTextField("40", 4);
         snapPan.add(lb);
         snapPan.add(snapSize);

         prefPan.add(snapPan);

         ctlPan = new JPanel();
         ctlPan.setLayout(new SimpleH2Layout(SimpleH2Layout.LEFT, 5, 0, false));
         lb = new JLabel("Template:");
         ctlPan.add(lb);
         tempList = new JComboBox();
         tempList.setEditable(true);
         tempList.setModel(new DefaultComboBoxModel());
         ctlPan.add(tempList);

         saveButton = addButton("Save", ctlPan, this, "saveTemplate");
         addButton("Load", ctlPan, this, "loadTemplate");
         addButton("Remove", ctlPan, this, "removeTemplate");
         addButton("Preview", ctlPan, this, "preview");
         addButton("Close", ctlPan, this, "close");

         infoPan.add(headerPan);
         infoPan.add(attrScroll);
         infoPan.add(prefPan);
         infoPan.add(ctlPan);
        
         infoPan.setLayout(new annxLayout());
         initActions();
    }


    /* loading imaging parameters */
    private void buildParamSource() {
         String path = "USER/PROPERTIES/parlist";
         String fpath = FileUtil.openPath(path);
         BufferedReader fin;
         int s, t;
         String data, p, type, v;
         StringTokenizer tok;
         if (fpath == null) {
             fpath = FileUtil.openPath("SYSTEM/imaging/templates/vnmrj/properties/parlist");
             if (fpath == null)
                fpath = FileUtil.openPath("PROPERTIES/parlist");
         }
         parList.removeAllItems();
         bNewParm = true;
         if (fpath != null) {
             try {
                fin = new BufferedReader(new FileReader(fpath));
                while ((data = fin.readLine()) != null) {
                   if (data.startsWith("#"))
                      continue;
                   tok = new StringTokenizer(data, " ,\t\r\n");
                   if (tok.countTokens() < 3)
                      continue;
                   p = tok.nextToken();
                   type = tok.nextToken();
                   t = 1;  // integer
                   if (type.equals("string"))
                      t = 3;
                   else if (type.equals("float"))
                      t = 2;
                   v = tok.nextToken();
                   try {
                       s = Integer.parseInt(v);
                   }
                   catch (NumberFormatException e) {
                       s = 0;
                   }
                   if (s > 0) {
                      ParamObj obj = new ParamObj(p, s, t);
                      parList.addItem(obj);
                   }
                }
                fin.close();
             } 
             catch(IOException e) {}
         }
         if (parList.getItemCount() > 0) {
             parLabel.setVisible(true);
             parList.setVisible(true);
         }
         else {
             parLabel.setVisible(false);
             parList.setVisible(false);
         }
         bNewParm = false;
    }

    private void getActiveEntry() {
         Component focusOwner =
            KeyboardFocusManager.getCurrentKeyboardFocusManager().
                getFocusOwner();
        activeEntry = null;
        if (focusOwner == null) {
            return;
        }
        if (focusOwner instanceof JTextField) {
            for (int n = 0; n < 20; n++) {
               if (focusOwner == varObjs[n]) {
                    activeEntry = (JTextField) varObjs[n];
                    break;
               }
            }
        }
        if (activeEntry != null) {
            bgColor = activeEntry.getBackground();
            activeEntry.setBackground(Color.yellow);
            activeEntry.repaint();
        }
    }

    private void setEntryActive() {
        if (activeEntry != null) {
           activeEntry.setBackground(bgColor);
           activeEntry.requestFocus();
        }
    }


    private void removeTemplate(String name) {
         tempList.removeItem(name);
         if (fList == null)
            return;
         int vsize = fList.size();
         for (int k = 0; k < vsize; k++) {
             String s = (String) fList.elementAt(k);
             if (name.equals(s)) {
                 fList.removeElementAt(k);
                 break;
             }
         }
    }

    private void addTemplate(String name) {
         if (fList == null)
            fList = new Vector();
         boolean newFile = true;
         int vsize = fList.size();
         for (int k = 0; k < vsize; k++) {
             String s = (String) fList.elementAt(k);
             if (name.equals(s)) {
                 newFile = false;
                 break;
             }
         }
         if (newFile) {
              fList.add(name);
              tempList.addItem(name);
         }
    }

    private void addTemplateList(String path, boolean sys) {
         if (path == null)
             return;
         File dir = new File(path);
         if (!dir.exists())
            return;
         if (!dir.isDirectory())
            return;

         File fd;
         int len;
         boolean newFile = false;
         String name;
         String files[] = dir.list();
         for (int i = 0; i < files.length; i++) {
             name = files[i];
             len = name.length();
             newFile = false;
             if (name.endsWith(".xml") && (len > 4)) {
                fd = new File(dir.getPath(), name);
                if (fd.isFile() && fd.canRead()) {
                   String f = name.substring(0, len - 4);
/*
                   if (!sys)
                      fList.add(f);
                   else {          
*/
                      int p = -1;
                      int c = 1;
                      int vsize = fList.size();
                      for (int k = 0; k < vsize; k++) {
                          String s = (String) fList.elementAt(k);
                          c = s.compareTo(f);
                          if (c == 0)
                              break;
                          if (c > 0) {
                              p = k;
                              break;
                          }
                      }
                      if (p >= 0)
                          fList.add(p, f);
                      else if (c != 0)
                          fList.add(f);
/*
                   }
*/
                }
             }
         }
    }

    private void buildTemplateList() {
         if (fList == null)
            fList = new Vector();
         else
            fList.clear();
         addTemplateList(FileUtil.openPath(userPath), false);
         sysPath = "SYSTEM/imaging/templates/vnmrj/annotation/";
         addTemplateList(FileUtil.openPath(sysPath), true);
         tempList.removeAllItems();
         String s;
         int vsize = fList.size();
         int k;
         for (k = 0; k < vsize; k++) {
             s = (String) fList.elementAt(k);
             tempList.addItem(s);
         }
         String path;
         PrintWriter os = null;
         s = userPath + "annotationlist";
         path = FileUtil.savePath(s);
         if (path != null) {
             try {
               os = new PrintWriter( new OutputStreamWriter(
                   new FileOutputStream(path), "UTF-8"));
             }
             catch(IOException er) { return; }
             for (k = 0; k < vsize; k++) {
                s = (String) fList.elementAt(k);
                os.println("\""+s+"\""+"  "+"\""+s+"\"");
             }
             os.close();
         }
    }

    private void initActions() {
         snapSize.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                String s = snapSize.getText().trim();
                if (s.length() < 1)
                    s = "40";
                int k = 1;
                try {
                    k = Integer.parseInt(s);
                }
                catch (NumberFormatException e) {
                    k = 0;
                }
                if (k < 0) {
                    snapSize.setText("0");
                    k = 0;
                }
                gridNum = k;
                dispPan.setGridNum(k);
            }
        });
    }

    private JButton addButton(String label, Container p, Object obj, String cmd)
    {
        JButton but = new JButton(label);
        but.setActionCommand(cmd);
        but.addActionListener((ActionListener) obj);
       //  but.setFont(font);
        p.add(but);
        return but;
    }

    private void saveObj()
    {
       if (editVobj == null)
            return;
       if (editVobj instanceof VEditIF) {
            VEditIF obj = (VEditIF)editVobj;
            Object attrs[][] = obj.getAttributes();
            if (attrs == null)
                return;
            for (int i= 0; i< attrs.length; i++) {
                String str = null;
                int attrval=((Integer)attrs[i][0]).intValue();
                if (attrval != COUNT) {
                   if (varObjs[i] instanceof JTextField) {
                       str = ((JTextField)varObjs[i]).getText();
                   } else if (varObjs[i] instanceof JComboBox) {
                       str = (String)((JComboBox)varObjs[i]).getSelectedItem();
                   }
                   if(str !=null && str.length()>0)
                      obj.setAttribute(attrval, str);
                   else
                      obj.setAttribute(attrval,null);
                }
            }
       }
    }

    private void comboAction(ActionEvent e) {
        if (editVobj != null) {
            String cmd = e.getActionCommand();
            JComboBox combo = (JComboBox)e.getSource();
            String sel = (String)combo.getSelectedItem();
            try {
                int attr = Integer.parseInt(cmd);
                curAttrCode = attr;
                editVobj.setAttribute(attr, sel);
            } catch (NumberFormatException exception) {
                Messages.postError("Annotation.comboAction");
            }
        }
    }

    private void showObj()
    {
        attrPan.removeAll();
        if (editVobj == null) {
            attrPan.validate();
            return;
        }
        VEditIF vobj = (VEditIF)editVobj;
        Object attrs[][] = vobj.getAttributes();
        if (attrs == null)
            return;
        int nats = attrs.length;
        int h = nats * attrWidth;
        JComponent activeObj = null;

        for (int i = 0; i < nats; i++) {
            int attrCode = ((Integer)attrs[i][0]).intValue();
            if(attrs[i].length < 2) {
                continue;
            }
            if (varPans[i] == null)
                varPans[i] = new JPanel();
            else
                varPans[i].removeAll();
            if (varLabels[i] == null)
                varLabels[i] = new JLabel(((String)attrs[i][1]).trim());
            else
                varLabels[i].setText((String)attrs[i][1]);
            varPans[i].add(varLabels[i]);
            String data = vobj.getAttribute(attrCode);
            String type = "text";
            if(attrs[i].length > 2){
                type = (String)attrs[i][2];
            }
            if (type.equals("text")) {
                varPans[i].setLayout(xLayout);
                JTextField jtxt = new JTextField("", 20);
                varObjs[i] = jtxt;
                if (data != null)
                    jtxt.setText(data);
                jtxt.addActionListener(textListener);
                jtxt.setActionCommand(String.valueOf(attrCode));
                if (attrCode == curAttrCode)
                    activeObj = jtxt;
            }
            else if (type.equals("menu")||type.equals("style")||type.equals("color")) {

                // Use menu to take attribute value
                JComboBox jcombo;
                varPans[i].setLayout(tLayout);
                if (type.equals("menu"))
                    jcombo = new JComboBox((Object[])attrs[i][3]);
                else{
                    jcombo = new VAnnChooser(type);
                    if(type.equals("color") && data==null)
                        data="transparent";
                }
                varObjs[i] = jcombo;
                int nm = jcombo.getMaximumRowCount();
                if (nm < 10)
                    jcombo.setMaximumRowCount(10); 
                jcombo.setActionCommand(String.valueOf(attrCode));
                jcombo.addActionListener(new ActionListener() {
                    public void actionPerformed(ActionEvent evt) {
                        comboAction(evt);
                    }
                });
                if (data != null) {
                    int n = jcombo.getItemCount();
                    for (int j=0; j<n; j++) {
                        String str = ((String)jcombo.getItemAt(j));
                        if (data.equalsIgnoreCase(str)) {
                            jcombo.setSelectedIndex(j);
                            break;
                        }
                    }
                }
            } // menu
            if (varObjs[i] != null)
                varPans[i].add(varObjs[i]);
            attrPan.add(varPans[i]);
        }
        attrPan.validate();
        if (activeObj != null)
            activeObj.requestFocus();
            
    }

    public void setAttr(int key, String val)
    {
       if (editVobj == null)
            return;
       if (!(editVobj instanceof VEditIF))
            return;
       VEditIF obj = (VEditIF)editVobj;
       Object attrs[][] = obj.getAttributes();
       if (attrs == null)
           return;
       for (int i= 0; i< attrs.length; i++) {
           String str = null;
           int attrval=((Integer)attrs[i][0]).intValue();
           if (attrval == key) {
               if (varObjs[i] instanceof JTextField)
                   ((JTextField)varObjs[i]).setText(val);
               return;
           }
       }
    }



    public void setOrientation(boolean b) {
        orientCheck.setSelected(b);
    }

    public boolean isOriented() {
        return orientCheck.isSelected();
    }

    public void setEditObj(VObjIF obj) {
       if (editVobj == obj) {
            dispPan.setEditObj(obj);
            return;
       }
       if (editVobj != null) {
            editVobj.setEditStatus(false);
            saveObj();
       }
       if (obj != null) {
           if (!(obj instanceof VEditIF)) {
              obj = null;
           }
       }
       dispPan.setEditObj(obj);
       editVobj = obj;
       if (obj == null) {
           typeOfobj.setText("none");
           attrPan.removeAll();
           attrPan.repaint();
           return;
       }
       String type = obj.getAttribute(TYPE);
       if (obj instanceof VAnnotateBox) {
          if (((VAnnotateBox) obj).isOrientObj())
             type = "orientation";
          else
             type = "group";
       }
       if (type.equals("textline"))
          type = "textmessage";
       if (obj instanceof VCheck)
          type = "orientation";
       // typeOfobj.setText(obj.getAttribute(TYPE));
       typeOfobj.setText(type);
       String data = obj.getAttribute(FONT_STYLE);
       textStyle.setStyleObject(obj);

       if(data != null && textStyle.isType(data))
           textStyle.setType(data);
       else
           textStyle.setDefaultType();
       editVobj.setEditStatus(true);
/*
       if (obj instanceof VCheck)
          editVobj = null;
*/
       showObj();
    }

    private void adjustAttrPanelSize() {
        Dimension dim;
        int   h = 0;
        int   w = 0;
        int   k;
        int   n = attrPan.getComponentCount();
        for ( k = 0; k < n; k++) {
            Component m = attrPan.getComponent(k);
            dim = m.getPreferredSize();
            h += dim.height+2;
        }
        w = windowWidth - 8;
        if (h > scrollH)
            w = w - sbW;
        attrPan.setPreferredSize(new Dimension(w, h));
        attrPan.repaint();
    }

    private String getTemplateName() {
        String fname, tmp;

        fname = null;
        tmp = ((String) tempList.getSelectedItem());
        if (tmp == null || tmp.length() <= 0)
             return null;
        tmp = tmp.trim();
        if (!tmp.endsWith(".xml"))
             fname = tmp;
        else {
             if (tmp.length() < 5)
                 return null;
             fname = tmp.substring(0, tmp.length() - 4);
        }
        return fname;
    }

    private void insertParam(ParamObj s) {
         if (bNewParm || activeEntry == null)
             return;
         String d = "annPar('"+s.name+"'";
         if (s.size > 1)
           d = d + ", 1";
/*
         if (s.type == 1)
            d = d + "):$vi ";
         else if (s.type == 2)
            d = d + "):$vf ";
         else
            d = d + "):$vs ";
*/
         d = d + "):$VALUE ";
         int k = activeEntry.getCaretPosition();
         activeEntry.replaceSelection(d);
    }

    public void actionPerformed(ActionEvent  evt)
    {
       String cmd = evt.getActionCommand();
       String fname;
       String path, f;

       if (cmd.equals("close")) {
            if (debugMode)
               System.exit(0);
            else {
               if (curTemplate != null) {
                  cmd = "aipAnnotation='"+curTemplate+"'\n";
                  Util.sendToActiveView(cmd);
                  cmd = "jFunc("+REDRAW+")\n";
                  Util.sendToAllVnmr(cmd);
               }
               setVisible(false);
            }
            return;
       }
       if (cmd.equals("editStyles")) {
            Util.showDisplayOptions();
            return;
       }
       if (cmd.equals("deleteObj")) {
            if (editVobj == null)
                return;
            JComponent obj = (JComponent) editVobj;
            setEditObj(null);
            dispPan.deleteObj(obj);
            // obj.getParent().remove(obj);
            // dispPan.repaint();
            // dispPan.validate(); 
            return;
       }
       if (cmd.equals("deleteAll")) {
            setEditObj(null);
            dispPan.deleteAll(); 
            // dispPan.removeAll(); 
            // dispPan.validate(); 
            dispPan.repaint();
            return;
       }
       if (cmd.equals("saveTemplate")) {
            saveObj();
            fname = getTemplateName();
            if (fname == null)
                return;
            StringTokenizer tok = new StringTokenizer(fname, " \n");
            if (!tok.hasMoreTokens())
                return;
            fname = tok.nextToken();
            while (tok.hasMoreTokens()) {
                fname = fname + " "+tok.nextToken();
            }
            f = userPath+fname+".xml";
            path = FileUtil.savePath(f);
            if (path == null)
                return;
            File fd = new File(path);
            if (fd.exists()) {
                int rt = JOptionPane.showConfirmDialog(this,
                    "Do you want to overwrite "+fname+"? ",
                    "Save Template",
                    JOptionPane.YES_NO_OPTION);
                if (rt != 0)
                    return;
            } 
            dispPan.saveTemplate(path);
/*
            addTemplate(fname);
*/
            f = userPath+fname+".txt";
            path = FileUtil.savePath(f);
            if (path == null)
                return;
            dispPan.saveTemplate_txt(path);
            buildTemplateList();
            tempList.setSelectedItem(fname);
            curTemplate = fname;
            // cmd = "aipAnnotation='"+fname+"'\n";
            // Util.sendToActiveView(cmd);
            return;
       }
       if (cmd.equals("loadTemplate")) {
            loadSelectedTemplate();
            return;
       }
       if (cmd.equals("removeTemplate")) {
            fname = getTemplateName();
            if (fname == null)
                return;
            f = userPath + fname+".xml";
            path = FileUtil.openPath(f);
            if (path == null) {
                f = sysPath + fname+".xml";
                path = FileUtil.openPath(f);
                if (path != null) {
                    JOptionPane.showMessageDialog(this,
                     "You don't have permission to remove "+fname+".");
                }
                return;
            }
            int r = JOptionPane.showConfirmDialog(this,
                    "Do you want to remove "+fname+"? ",
                    "Remove Template",
                    JOptionPane.YES_NO_OPTION);
            if (r != 0)
                return;
            File tmpFile = new File(path);
            tmpFile.delete();
/*
            removeTemplate(fname);
*/
            f = userPath + fname+".txt";
            path = FileUtil.openPath(f);
            if (path != null) {
                tmpFile = new File(path);
                tmpFile.delete();
            }
            buildTemplateList();
            loadSelectedTemplate();
            return;
       }
       if (cmd.equals("preview")) {
            saveObj();
            if (!Util.isNativeGraphics()) {
                f = userPath + "annpreview.xml";
                path = FileUtil.savePath(f);
                if (path == null)
                    return;
                dispPan.saveTemplate(path);
                File tmpFile = new File(path);
                if (!tmpFile.exists())
                    return;
                ExpPanel exp = Util.getActiveView();
                if (exp != null)
                    exp.previewAnnotation(path);

                tmpFile.delete();
                return;
            }
            f = userPath + "annpreview.txt";
            path = FileUtil.savePath(f);
            if (path == null)
                return;
            dispPan.saveTemplate_txt(path);
            cmd = "annotation('-preview', '-d', '-f', 'annpreview')\n";
            Util.sendToActiveView(cmd);
            return;
       }
       if (cmd.equals("orientation")) {
            JCheckBox cb = (JCheckBox) evt.getSource();
            dispPan.show_orientation(cb.isSelected());
            return;
       }
    }


    public static void main(String[] args) {
         debugMode = true;
         try {
                // lookandfeel can be motif or metal
                String look = System.getProperty("lookandfeel");
                String lf = UIManager.getSystemLookAndFeelClassName();
                UIManager.LookAndFeelInfo[] lists = UIManager.getInstalledLookAndFeels();
                if (look != null) {
                    look = look.toLowerCase();
                    for (int k = 0; k < lists.length; k++) {
                        String e = lists[k].getName().toLowerCase();
                        if (e.indexOf(look) >= 0) {
                            lf = lists[k].getClassName();
                            break;
                        }
                    }
                }
                UIManager.setLookAndFeel(lf);
         } catch (Exception e) {}

         AnnotationEditor ed = new AnnotationEditor();
         ed.show();
    }

    class attrLayout implements LayoutManager {
        public void addLayoutComponent(String name, Component comp) {}
        public void removeLayoutComponent(Component comp) {}

        public Dimension preferredLayoutSize(Container target) {
            Dimension dim;
            int   w = 0;
            int   h = 0;
            int   k;
            int   n = target.getComponentCount();
            for ( k = 0; k < n; k++) {
                Component m = target.getComponent(k);
                dim = m.getPreferredSize();
                if (dim.width > w)
                    w = dim.width;
                h += dim.height+2;
            }
            return new Dimension(w, h);
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        }

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Dimension dim;
                int   n = target.getComponentCount();
                int   w = 0;
                int   h = 4;
                int   k;

                for ( k = 0; k < n; k++) {
                    Component m = target.getComponent(k);
                    if(m instanceof Container)
                        m=((Container)m).getComponent(0);
                    dim = m.getPreferredSize();
                    if (dim.width > w)
                        w = dim.width;
                }

                SimpleH2Layout.setFirstWidth(w);

                Dimension dim0 = target.getSize();
                for (k = 0; k < n; k++) {
                    Component obj = target.getComponent(k);
                    if (obj.isVisible()) {
                        dim = obj.getPreferredSize();
                        obj.setBounds(2, h, dim0.width-6, dim.height);
                        h += dim.height;
                    }
                }
                if (parLabel.isVisible()) {
                    Rectangle r = parList.getBounds();
                    k = dim0.width - r.x - r.width - 10;
                    if (k > 0) {
                         r.x += k;
                         parList.setBounds(r);
                         r = parLabel.getBounds();
                         r.x += k;
                         parLabel.setBounds(r);
                    }
                }
            }
        }
    } // class attrLayout


    private class ParamListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            JComboBox cb = (JComboBox) e.getSource();
            if (cb == null)
               return;
            ParamObj obj = (ParamObj) cb.getSelectedItem();
            if (obj == null)
               return;
            insertParam(obj);
        }
   }

    class TextChangeListener implements ActionListener {
       public void actionPerformed(ActionEvent e) {
           if(editVobj == null)
                return;
           String cmd = e.getActionCommand();
           JTextField obj = (JTextField) e.getSource();
           try {
                int attr = Integer.parseInt(cmd);
                curAttrCode = attr;
                editVobj.setAttribute(attr, obj.getText());
           }
           catch(NumberFormatException exception) {
                Messages.postError("Annotation number format exception");
           }
       }
    }

    class annxLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        public Dimension preferredLayoutSize(Container target) {
            Dimension dim;
            int   w = 0;
            int   h = 0;
            int   k;
            int   n = target.getComponentCount();
            for ( k = 0; k < n; k++) {
                Component m = target.getComponent(k);
                dim = m.getPreferredSize();
                if (dim.width > w)
                    w = dim.width;
                h += dim.height+2;
            }
            return new Dimension(w, h);
        }


        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(100, 20);
        }

        public void layoutContainer(Container target) {
            synchronized(target.getTreeLock()) {
               Dimension dim;
                int   n = target.getComponentCount();
                int   w = 0;
                int   h = 0;
                int   hs = 0;
                int   k;

                for ( k = 0; k < n; k++) {
                    Component m = target.getComponent(k);
                    dim = m.getPreferredSize();
                    if (m != attrScroll)
                    hs += dim.height;
                }

                Dimension dim0 = target.getSize();
                windowWidth = dim0.width;
                hs = dim0.height - hs - 4;
                if (hs < 20)
                    hs = 20;
                scrollH = hs;
                if (sbW == 0) {
                    JScrollBar jb = attrScroll.getVerticalScrollBar();
                    if (jb != null) {
                        dim = jb.getPreferredSize();
                        sbW = dim.width;
                    }
                    else
                        sbW = 20;
                }
                if (editVobj != null)
                    adjustAttrPanelSize();

                for (k = 0; k < n; k++) {
                    Component obj = target.getComponent(k);
                    if (obj.isVisible()) {
                        dim = obj.getPreferredSize();
                        if (obj == attrScroll) {
                           obj.setBounds(2, h, dim0.width-4, hs);
                           h += hs;
                        }
                        else {
                           obj.setBounds(2, h, dim0.width-4, dim.height);
                           h += dim.height;
                        }

                    }
                }
            } // synchronized
        } // layoutContainer
    }

    private class ParamObj extends Object {
        public String  name;
        public int     size;
        public int     type;

        public ParamObj(String s, int l, int t) {
            this.name = s;
            this.size = l;
            this.type = t;
        }

        public String toString() {
            if (size > 1)
               return name+" [1-"+size+"]";
            else
               return name;
        }
    }

    private class VInfoObj extends SmsInfoObj {
        public VInfoObj(String str, int n) {
            super(str, n);
        }

        public void setValue(ParamIF pf) {
           if (pf != null && pf.value != null) {
                initTemplate(pf.value);
           }
        }
    }
}
