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


import vnmr.ui.*;
import vnmr.bo.*;
import vnmr.admin.ui.*;
import vnmr.admin.util.*;

public class TextboxEditor extends JDialog
      implements ActionListener, VObjDef
{
    private JPanel ctlPan;
    private JPanel fontPan;
    private JScrollPane textPan;
    private JTextArea textComp;
    private VColorChooser  colorChoice;
    // private XJComboBox sizeChoice;
    // private XJComboBox styleChoice;
    private JComboBox  sizeChoice;
    private VStyleChooser styleChooser;
    private VFontChooser fontChooser;
    private String colorStr = "cyan";
    private String sizeStr = "12";
    private String styleStr = "Plain";
    private String fontName = Font.DIALOG;
    private int[]  tflags;
    private int    tnum = 0;
    private boolean  bSetNewObj = false;
    private XJButton newBut;
    private XJButton updateBut;
    private XJButton deleteBut;
    private XJButton closeBut;
    private VTextBox textObj = null;
    private Color defaultCaretColor;
    private SessionShare sshare;
    private static TextboxEditor editor = null;
    private Runnable updateRunner = null;
    public static String userPath = new StringBuffer().append("USER").
                append(File.separator).append("TEXTBOX").append(File.separator).
                append("templates").append(File.separator).toString();
    public static String sysPath = new StringBuffer().append("SYSTEM").
                append(File.separator).append("TEXTBOX").append(File.separator).
                append("templates").append(File.separator).toString();
    public static String tmpPath = new StringBuffer().append("USER").
                append(File.separator).append("VNMRJ").append(File.separator).
                append("textbox").append(File.separator).append("tmp").append(File.separator).toString();
    
    public static String tbFontName = "txtBoxFntName";
    public static String tbFontStyle = "txtBoxFntStyle";
    public static String tbFontSize = "txtBoxFntSize";
    public static String tbFontColor = "txtBoxFntColor";

    public TextboxEditor() {
         super(VNMRFrame.getVNMRFrame(), "Text Editor");
         setBackground(Util.getBgColor());
         createPanels();

         tnum = 30;
         tflags = new int[tnum];
         for (int n = 0; n < tnum; n++)
            tflags[n] = 0;

         // Toolkit tk = Toolkit.getDefaultToolkit();
         // Dimension screenDim = tk.getScreenSize();
         int h = 300;
         int w = 520;
         setSize(w, h);
         setLocation(60, 0);
         validate();
         // setupUserDir();
         updateTextCompBg();
    }

    public static TextboxEditor defaultEditor() {
         if (editor == null)
             editor = new TextboxEditor();
         return editor;
    }

    public static void execCmd(String cmd) {
         boolean bOpen = true;
         if (editor == null)
             editor = new TextboxEditor();
         if (cmd != null && (cmd.length() > 0)) {
             if (cmd.equals("close"))
                 bOpen = false;
         }
         if (bOpen) {
             editor.setVisible(true);
         }
         else
             editor.setVisible(false);
    }

    private void setupUserDir() {
         String p = FileUtil.savePath(userPath);
         if (p == null)
             return;
         String sp = FileUtil.openPath(sysPath);
         if (sp == null)
             return;
         File userDir = new File(p);
         if (!userDir.exists())
            return;
         File sysDir = new File(sp);
         if (!sysDir.exists())
            return;
         String userFiles[] = userDir.list();
         String sysFiles[] = sysDir.list();
         String sName, uName;
         int n1, n2, k1, k2;
         boolean bExist;
         n1 =  sysFiles.length;
         n2 =  userFiles.length;

         for (k1 = 0; k1 < n1; k1++) {
             sName = sysFiles[k1];
             bExist = false;
             for (k2 = 0; k2 < n2; k2++) {
                 uName = userFiles[k2];
                 if (uName.equals(sName)) {
                     bExist = true;
                     break;
                 }
             }
         }
    }

    private void updateTextCompBg() {
         ExpPanel exp = Util.getActiveView();
         if (exp == null)
             return;
         VnmrCanvas canvas = exp.getCanvas();
         Color bg = canvas.getBackground();
         textComp.setBackground(new Color(bg.getRGB()));
         if (bg.getRed() < 80 && bg.getGreen() < 80)
            textComp.setCaretColor(Color.white);
         else
            textComp.setCaretColor(defaultCaretColor);
    }
         
    private void updateTextComp() {
         int style = Util.fontStyle(styleStr);
         int size = 12;
         try {
              size = Integer.parseInt(sizeStr);
         }
         catch (NumberFormatException e) {}

         Font f = new Font(fontName, style ,size);
         textComp.setFont(f);
         if (colorStr != null) {
            Color c = DisplayOptions.getColor(colorStr);
            textComp.setForeground(c);
         }
    }

    private void checkEditObj() {
         boolean bActive = true;

         if (textObj == null)
            bActive = false;
         else {
            if (!textObj.isActive()) {
               bActive = false;
               textObj = null;
            }
         }
         deleteBut.setEnabled(bActive);
         updateBut.setEnabled(bActive);
    }


    private void updateEditObj() {
         if (textObj == null)
             return;
         String s = textObj.getSrcData();
         if (s != null)
             textComp.setText(s);
         else
             textComp.setText("");
         bSetNewObj = true;
         s = textObj.getFontStyle();
         if (s != null) {
             styleStr = s;
             // if (styleChoice != null)
             //    styleChoice.setSelectedItem(s);
             if (styleChooser != null)
                styleChooser.setSelectedItem(s);
         }
         s = textObj.getFontSize();
         if (s != null) {
             sizeStr = s;
             sizeChoice.setSelectedItem(s);
         }
         s = textObj.getColorName();
         if (s != null) {
             colorStr = s;
             colorChoice.setColor(s);
             colorChoice.repaint();
         }
         s = textObj.getFontName();
         if (s != null) {
             fontName = s;
             fontChooser.setSelectedItem(s);
         }

         deleteBut.setEnabled(true);
         updateBut.setEnabled(true);
         updateTextCompBg();
         updateTextComp();
         bSetNewObj = false;
    }

    private void setupEditObj(VTextBox comp) {
         textObj = comp;
         checkEditObj();
         if (textObj == null)
             return;
         if (updateRunner == null) {
             updateRunner = new Runnable() {
                public void run() {
                    updateEditObj();
                }
             };
         }
         SwingUtilities.invokeLater(updateRunner);
    }

    public static void setEditObj(VTextBox comp) {
         if (editor == null)
             return;
         editor.setupEditObj(comp);
    }

    public static void updateEditMode() {
         if (editor == null)
             return;
         editor.checkEditObj();
    }

    private void saveDefaultAttrs() {
         if (sshare == null)
            return;
         Hashtable hs = sshare.userInfo();
         if (hs == null)
            return;
         hs.put(tbFontColor, colorStr);
         hs.put(tbFontSize, sizeStr);
         hs.put(tbFontName, fontName);
         hs.put(tbFontStyle, styleStr);
    }

    private void getDefaultAttrs() {
         sshare = Util.getSessionShare();
         if (sshare == null)
            return;
         Hashtable hs = sshare.userInfo();
         if (hs == null)
            return;
         String data = (String) hs.get(tbFontColor);
         if (data != null)
            colorStr = data;
         data = (String) hs.get(tbFontSize);
         if (data != null)
            sizeStr = data;
         data = (String) hs.get(tbFontName);
         if (data != null)
            fontName = data;
         data = (String) hs.get(tbFontStyle);
         if (data != null)
            styleStr = data;
    }

    private void createPanels() {
         getDefaultAttrs();
         setLayout(new BorderLayout(0, 10));
         JLabel lb = new JLabel("Text:");
         add(lb, BorderLayout.NORTH);
         textPan = new JScrollPane();
         textComp = new JTextArea("");
         textComp.setMargin(new Insets(4, 4, 4, 4));
         textComp.setBackground(Util.getBgColor());
         textPan.setViewportView(textComp);
         add(textPan, BorderLayout.CENTER);
         defaultCaretColor = textComp.getCaretColor();

         fontPan = new JPanel();
         fontPan.setLayout(new SimpleH2Layout(SimpleH2Layout.LEFT, 5, 0, false));
         lb = new JLabel("Font: ");
         fontPan.add(lb);
         fontChooser = new VFontChooser(sshare, null, "font");
         fontPan.add(fontChooser);
         fontChooser.setAttribute(VObjDef.VALUE, fontName);
         fontChooser.addActionListener(this);

         styleChooser= new VStyleChooser(sshare,null,"style");
         styleChooser.setAttribute(VObjDef.VALUE, styleStr);
         fontPan.add(styleChooser);
         styleChooser.addActionListener(this);

         // lb = new JLabel("Color:");
         // fontPan.add(lb);
         colorChoice = new VColorChooser(sshare, null, "color");
         colorChoice.setColor(colorStr);
         colorChoice.addActionListener(this);
         // fontPan.add(colorChoice);
         // lb = new JLabel("Size:");
         // fontPan.add(lb);
         sizeChoice = new JComboBox();
         sizeChoice.addItem("8");
         sizeChoice.addItem("10");
         sizeChoice.addItem("12");
         sizeChoice.addItem("14");
         sizeChoice.addItem("16");
         sizeChoice.addItem("18");
         sizeChoice.addItem("24");
         sizeChoice.addItem("32");
         sizeChoice.addItem("Variable");
         sizeChoice.setSelectedItem(sizeStr);
         sizeChoice.setActionCommand("size");
         sizeChoice.addActionListener(this);
         fontPan.add(sizeChoice);

         /***
         lb = new JLabel("Style:");
         fontPan.add(lb);
         styleChoice = new XJComboBox();
         styleChoice.addActionListener(this);
         styleChoice.setActionCommand("style");
         styleChoice.addItem("Plain");
         styleChoice.addItem("Bold");
         styleChoice.addItem("PlainItalic");
         styleChoice.addItem("BoldItalic");
         fontPan.add(styleChoice);
         ***/

         fontPan.add(colorChoice);

         ctlPan = new JPanel();
         ctlPan.setLayout(new SimpleH2Layout(SimpleH2Layout.CENTER, 16, 0, false));
         newBut = addButton("New", ctlPan, this, "new");
         updateBut = addButton("Update", ctlPan, this, "update");
         deleteBut = addButton("Delete", ctlPan, this, "delete");
         closeBut = addButton("Close", ctlPan, this, "close");

         JPanel infoPan = new JPanel();
         infoPan.setLayout(new SimpleVLayout(0, 20, 10, 10, false));
         infoPan.add(fontPan);
         infoPan.add(ctlPan);
        
         updateBut.setEnabled(false);
         deleteBut.setEnabled(false);
         add(infoPan, BorderLayout.SOUTH);
    }



    private XJButton addButton(String label, Container p, Object obj, String cmd)
    {
        XJButton but = new XJButton(label);
        but.setTestMode(true);
        but.setActionCommand(cmd);
        but.addActionListener((ActionListener) obj);
        but.setBorderInsets(new Insets(4, 10, 4, 10)); // top,left,bottom,right
        but.set3D(false);
        but.setBackground(but.getBackground());
        p.add(but);
        return but;
    }

    private void menuAction(JComponent comp) {
        JComboBox cb = (JComboBox) comp;
        String s = (String) cb.getSelectedItem();
        if (comp.equals(sizeChoice)) {
            sizeStr = s;
            return;
        }
        if (comp.equals(styleChooser)) {
            styleStr = s;
            return;
        }
        if (comp.equals(fontChooser)) {
            fontName = s;
            return;
        }

        // if (comp.equals(styleChoice)) {
        //     styleStr = s;
        //     return;
        // }
    }


    public void actionPerformed(ActionEvent  evt)
    {
       JComponent cb = (JComponent) evt.getSource();
       String cmd = evt.getActionCommand();
       if (cmd.equals("menu") || cmd.equals("button")) {
            String c = colorChoice.getAttribute(VObjDef.VALUE);
            colorStr = c;
            if (!bSetNewObj)
                updateTextComp();
            return;
       }
       if (cb instanceof JComboBox) {
            menuAction(cb);
            if (!bSetNewObj)
               updateTextComp();
            return;
       }
       if (!(cb instanceof JButton)) {
            return;
       }
 
       if (cmd.equals("close")) {
            saveDefaultAttrs();
            updateBut.setEnabled(false);
            deleteBut.setEnabled(false);
            setVisible(false);
            return;
       }
       if (cmd.equals("delete")) {
            if (textObj != null)
                textObj.delete(); 
            textObj = null;
            checkEditObj();
            return;
       }
       String s = textComp.getText().trim();
       if (s.length() <= 0)
           return;
       s = textComp.getText();

       if (cmd.equals("update")) {
           if (textObj == null || !textObj.isActive())
               return;
           textObj.setFontInfo(fontName, styleStr, sizeStr);
           textObj.setColorName(colorStr);
           textObj.setSrcData(s);
           textObj.update();
           return;
       }
       if (cmd.equals("new")) {
           int n = 0;
           ExpPanel exp = (ExpPanel) Util.getActiveView();
           if (exp == null)
                return;

           for (n = 0; n < tnum; n++) {
               if (tflags[n] == 0) {
                  tflags[n] = 1;
                  break;
               }
           }
           if (n >= tnum) {
               tflags = new int[tnum+10];
               for (n = 0; n <= tnum; n++)
                  tflags[n] = 1;
               for (n = tnum+1; n < tnum+10; n++)
                  tflags[n] = 0;
               n = tnum;
               tnum = n + 10; 
           }
           String name = "tmp"+n;
           String f = tmpPath+name;
           String path = FileUtil.savePath(f); 
             
           if (path == null)
                return;
           VnmrCanvas canvas = exp.getCanvas();
           textObj = new VTextBox();
           textObj.setFontInfo(fontName, styleStr, sizeStr);
           textObj.setColorName(colorStr);
           textObj.setSrcData(s);
           textObj.setSrcFileDir(tmpPath);
           textObj.setSrcFileName(name);
           textObj.setSaveFileDir(tmpPath);
           textObj.setSaveFileName(name);
           canvas.addTextBox(textObj);
           updateBut.setEnabled(true);
           deleteBut.setEnabled(true);
           return;
       }
    }

    public static String getTemplateFile(String temp, String file,
                          boolean bUserOnly, boolean bSave) {
         String sp = FileUtil.openPath(userPath);
         String p = new StringBuffer().append(sp).append(temp).
                        append(File.separator).append(file).toString();
         String fp;
         if (bSave)
             fp = FileUtil.savePath(p); 
         else
             fp = FileUtil.openPath(p); 
         if (fp != null)
             return fp;
         if (bUserOnly)
             return null;
         
         sp = FileUtil.openPath(sysPath);
         p = new StringBuffer().append(sp).append(temp).
                        append(File.separator).append(file).toString();
         if (bSave)
             fp = FileUtil.savePath(p); 
         else
             fp = FileUtil.openPath(p); 
         return fp;
    }

    
    public static void deleteTemplate(String f) {
         String p = FileUtil.openPath(userPath);
         String fp;
         if (p.endsWith(File.separator))
             fp = p+f;
         else
             fp = p+File.separator+f;
         File fd = new File(fp);
         if (fd == null || !fd.exists() || !fd.canWrite())
             return;
         String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, "rm -rf "+fp};
         WUtil.runScriptInThread(cmd);
    }

    public static void saveTemplate(String temp, Vector v) {
         if (v == null || v.size() < 1)
             return;
         String p = userPath+temp+File.separator+"info";
         String fp = FileUtil.savePath(p);
         PrintWriter os = null;
         PrintWriter txtOs = null;
         if (fp == null)
             return;
         try {
              os = new PrintWriter( new OutputStreamWriter(
              new FileOutputStream(fp), "UTF-8"));
         }
         catch(IOException er) { return; }
         int num = v.size();
         int k = 1;
         VTextBox obj;
         String txtName;
         String data;
         
         for (int i = 0; i < num; i++) {
              obj = (VTextBox) v.elementAt(i); 
              if (obj == null)
                  continue;
              if (obj.isTextFrameType())
                  continue;
              if (!obj.isVisible())
                  continue;
              data = obj.getSrcData();
              if (data == null)
                  continue;
              data = data.trim();
              if (data.length() < 1)
                  continue;
              data = obj.getSrcData();
              os.println("textbox: "+k);
              os.println("color: "+obj.getColorName());
              os.println("fontname: "+obj.getFontName());
              os.println("fontsize: "+obj.getFontSize());
              os.println("fontstyle: "+obj.getFontStyle());
              os.println("x: "+obj.getX());
              os.println("y: "+obj.getY());
              os.println("width: "+obj.getWidth());
              os.println("height: "+obj.getHeight());
              txtName = obj.getSrcFileName();
              if (txtName == null)
                  txtName = "box";
              if (!txtName.equals("text")) {
                  txtName = "box"+k;
                  p = userPath+temp+File.separator+txtName;
                  fp = FileUtil.savePath(p);
                  txtOs = null;
                  try {
                      if (fp != null)
                          txtOs = new PrintWriter( new OutputStreamWriter(
                              new FileOutputStream(fp), "UTF-8"));
                  }
                  catch(IOException e1) { txtOs = null; }
                  if (txtOs != null) {
                      txtOs.print(data); 
                      txtOs.close();
                  }
              }
              os.println("textfile: "+txtName);
              k++;
         }
         os.close();
    }

    public static void loadTemplate(ButtonIF vif, String temp, Vector v) {
         if (v == null)
             return;
         String fp = tmpPath+"tmp0";
         fp = FileUtil.savePath(fp); // open user tmp directory

         fp = getTemplateFile(temp, "info", false, false);
         if (fp == null)
             return;
         File fd = new File(fp);
         if (fd == null || !fd.exists() || !fd.canRead())
             return;
         String dirP = fd.getParent();
         String usrDir = null;
         String usrFp = getTemplateFile(temp, "info", true, true);
         if (usrFp != null) {
             usrFp = FileUtil.openPath(userPath);
             usrDir = new StringBuffer().append(usrFp).append(temp).
                           append(File.separator).toString();
         }
         // v.clear();
         int num = 1;
         int k = 1;
         String p;
         boolean bTextExist = false;
         VTextBox obj = null;
         StringTokenizer tok;
         BufferedReader  fin = null;
         for (k = 0; k < v.size(); k++) {
             obj = (VTextBox) v.elementAt(k);
             if (obj != null) {
                 if (obj.getId() >= num)
                    num = obj.getId() + 1;
             }
         }

         obj = null;
         try {
             fin = new BufferedReader(new FileReader(fp));
             if (fin == null)
                 return;
             while ((p = fin.readLine()) != null) { 
                 tok = new StringTokenizer(p, " ,\n\t");
                 if (!tok.hasMoreTokens())
                    continue;
                 fp = tok.nextToken();
                 if (fp.length() < 2)
                    continue;
                 if (fp.startsWith("textbox")) {
                      if (obj != null) {
                          if (bTextExist) {
                              v.addElement(obj);
                              obj.readSource();
                              obj.setupFont();
                          }
                          bTextExist = false;
                          obj = null;
                      }
                      continue;
                 }
                 if (!tok.hasMoreTokens())
                      continue;
                 if (obj == null) {
                      obj = new VTextBox();
                      obj.setId(num);
                      obj.vnmrIf = vif;
                      obj.setSrcFileDir(dirP);
                      obj.setTemplateName(temp);
                      obj.setSaveFileDir(usrDir);
                      num++;
                 }
                 if (fp.startsWith("color:")) {
                      obj.setColorName(tok.nextToken());
                      continue;
                 }
                 if (fp.startsWith("fontname")) {
                      obj.setFontName(tok.nextToken());
                      continue;
                 }
                 if (fp.startsWith("fontsize")) {
                      obj.setFontSize(tok.nextToken());
                      continue;
                 }
                 if (fp.startsWith("fontstyle")) {
                      obj.setFontStyle(tok.nextToken());
                      continue;
                 }
                 if (fp.startsWith("textfile")) {
                      obj.setSrcFileName(tok.nextToken());
                      bTextExist = true;
                      continue;
                 }
                 if (fp.startsWith("x:")) {
                      k = Integer.parseInt(tok.nextToken());
                      obj.setX(k);
                      continue;
                 }
                 if (fp.startsWith("y:")) {
                      k = Integer.parseInt(tok.nextToken());
                      obj.setY(k);
                      continue;
                 }
                 if (fp.startsWith("width")) {
                      k = Integer.parseInt(tok.nextToken());
                      obj.setWidth(k);
                      continue;
                 }
                 if (fp.startsWith("height")) {
                      k = Integer.parseInt(tok.nextToken());
                      obj.setHeight(k);
                      continue;
                 }
             }
         }
         catch(IOException e)
         { }
         if (fin != null) {
              try {
                   fin.close();
              } catch(IOException ee) { }
         }
         if (obj != null && bTextExist) {
              v.addElement(obj);
              obj.readSource();
              obj.setupFont();
         }
         for (int i = 0; i < v.size(); i++) {
              obj = (VTextBox) v.elementAt(i); 
              if (obj != null)
                  obj.update();
         }
    }

}
