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
import javax.swing.*;
import javax.swing.text.*;

import vnmr.util.*;

public class DpsDataPanel extends JPanel {
     private JLabel itemLabel;
     private JLabel attrLabel;
     private JTextPane txtPane;
     private JScrollPane scrollPane;

     public DpsDataPanel() {
          buildGUi();
     }

     private void buildGUi() {
          setLayout(new BorderLayout());
          JPanel pan = new JPanel();
          pan.setLayout(new SimpleVLayout());
          itemLabel = new JLabel("  ");
          itemLabel.setHorizontalAlignment(SwingConstants.CENTER);
          pan.add(itemLabel);
          attrLabel = new JLabel("  ");
          // attrLabel.setHorizontalAlignment(SwingConstants.CENTER);
          pan.add(attrLabel);
          JPanel toolPan = DpsUtil.getToolPanel();
          if (toolPan != null) {
              Dimension dim = toolPan.getPreferredSize();
              Dimension dim2 = pan.getPreferredSize();
              if (dim.height != dim2.height) {
                  if (dim2.height > dim.height) {
                      dim.height = dim2.height;
                      toolPan.setPreferredSize(dim);
                  }
                  else {
                      dim2.height = dim.height;
                      pan.setPreferredSize(dim2);
                  }
              }
          }
          txtPane = new JTextPane();
          txtPane.setEditable(false);
          scrollPane = new JScrollPane(txtPane,
               JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
               JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);

          add(pan, BorderLayout.NORTH);
          add(scrollPane, BorderLayout.CENTER);
     }

     public void setObjName(String str) {
          itemLabel.setText(str);
     }

     public void setObjAttr(String str) {
          attrLabel.setText(str);
     }

     public void setLabel(String str) {
          itemLabel.setText(str);
     }

     public void clear() {
         itemLabel.setText(" ");
         attrLabel.setText(" ");
         txtPane.setText(" ");
     }
  
     public void setFileData(String str, boolean bAppend) {
         String path =  FileUtil.openPath(str);
         if (path == null)
              return;
         BufferedReader fin = null;
         StringBuffer sbData = new StringBuffer();
         String line;
         try {
            fin = new BufferedReader(new FileReader(path));
            while ((line = fin.readLine()) != null) {
                 sbData.append(line).append("\n");
            }
         }
         catch(IOException ee) { }
         finally {
            try {
               if (fin != null)
                   fin.close();
            }
            catch (Exception e2) {}
         }
         if (bAppend) {
             Document doc = txtPane.getDocument();
             int len = doc.getLength();
             StyleContext sc = StyleContext.getDefaultStyleContext();
             AttributeSet attr = sc.addAttribute(SimpleAttributeSet.EMPTY,
                      StyleConstants.Foreground, Color.blue);

             try {
                 doc.insertString(len, sbData.toString(), attr);
              }
              catch (BadLocationException e) { }
         }
         else
             txtPane.setText(sbData.toString());
         txtPane.setCaretPosition(0);
     }

     public void setFileData(String str) {
         setFileData(str, false);
     }

     public void showDpsObjInfo(DpsObj obj) {
         clear();
         if (obj == null)
             return;
         String str = obj.getName();
         if (str != null) 
             setObjName(str);
         str = obj.getPattern();
         if (str != null)
            setObjAttr("Pattern: "+str);
         String path = obj.getPatternPath();
         if (path != null)
            setFileData(path, false);
         DpsObj dualObj = obj.getDualObject();
         if (dualObj != null) {
            str = dualObj.getPatternPath();
            if (str != null)
                setFileData(str, true);
         }
         if (path != null)
            return;
         str = obj.getInfo();
         if (str != null)
            txtPane.setText(str);
     }

}

