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
import java.awt.event.*;
import java.awt.Frame.*;
import java.util.*;
import java.io.*;
import javax.swing.*;

import vnmr.util.*;
import vnmr.bo.VColorChooser;
import vnmr.bo.VObjDef;

public class DpsOptions extends ModelessDialog implements ActionListener{
     private JPanel panel;
     private JPanel rfShapePanel;
     private JScrollPane    scrollPane;
     protected VColorChooser[] channelColors;
     protected JCheckBox[]    visibleCks;
     protected JCheckBox      durationCk;
     protected JRadioButton   absShapeRadio;
     protected JRadioButton   realShapeRadio;
     protected OptionPanel[]  optPanels;
     protected JComboBox  lwCb;
     protected int channelNum = 13;
     protected int baseLineId = 12;
     protected int selectedId = 12;
     protected int durationId = 12;
     protected int lineWidth = 1;
     private static final String RFSHAPE = "rfshape";

     public DpsOptions(Frame frame) {
          super(frame, "Dps Options");
          buildUi();
     }

     public DpsOptions() {
          super((Frame) null, "Dps Options");
          buildUi();
     }

     private void buildUi() {
          setSize(300, 442);
          setLocation(600, 200);
          baseLineId = DpsWindow.DPSCHANNELS;
          selectedId = baseLineId + 1;
          durationId = baseLineId + 2;
          channelNum = durationId + 1;
          initOptions();
          panel = new JPanel();
          panel.setLayout(new SimpleVLayout()); 
          scrollPane = new JScrollPane(panel,
                JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
          optPanels = new OptionPanel[channelNum];
          optPanels[1] = new OptionPanel(1);
          for (int n = 1; n < channelNum; n++) {
               optPanels[n] = new OptionPanel(n);
               panel.add(optPanels[n]);
          }
          optPanels[baseLineId].setLabel("Base Line");
          optPanels[selectedId].setLabel("Selected Color");
          optPanels[durationId].setLabel("Show Duration");

          panel.add(rfShapePanel);

          JPanel lwPan = new JPanel();
          lwPan.setLayout(new SimpleHLayout());
          JLabel lb = new JLabel(" Line Width: ");
          lwPan.add(lb);
          lwCb = new JComboBox();
          lwCb.addItem("1");
          lwCb.addItem("2");
          lwCb.addItem("3");
          lwCb.addItem("4");
          lwPan.add(lwCb);
          panel.add(lwPan);

          Container c = getContentPane();
          c.add(scrollPane, BorderLayout.CENTER);

          addWindowListener(new WindowAdapter() {
              public void windowClosing(WindowEvent we) {
                 closeProc();
              }
          });
          closeButton.setActionCommand("close");
          closeButton.addActionListener(this); 
          undoButton.setVisible(false);
          historyButton.setVisible(false);
          abandonButton.setVisible(false);
          helpButton.setVisible(false);

          if (lineWidth != 1)
              lwCb.setSelectedItem(Integer.toString(lineWidth));
          lwCb.addActionListener(new ActionListener() {
              public void actionPerformed(ActionEvent e) {
                 lwSelection();
              }
          });
     }

     private String getOptionFile() {
         String path = new StringBuffer().append("USER").append(
                        File.separator).append("PERSISTENCE").append(
                        File.separator).append("DpsOptions").toString();
         path = FileUtil.savePath(path, false);
         return path;
     }

     private void parseProperty(String str) {
         StringTokenizer tok = new StringTokenizer(str, " \t\r\n");
         if (!tok.hasMoreTokens())
              return;
         String type = tok.nextToken();
         String str1 = null;
         String str2 = null;
         if (!tok.hasMoreTokens())
              return;
         str1 = tok.nextToken();
         if (!tok.hasMoreTokens())
              return;
         str2 = tok.nextToken();
         int v1, v2;
         try {
             v1 = Integer.parseInt(str1);
             v2 = Integer.parseInt(str2);
             if (type.equals("#loc")) {
                 Dimension d = Toolkit.getDefaultToolkit().getScreenSize();
                 if (v1 > (d.width - 200))
                     v1 = d.width - 200;
                 if (v2 > (d.height - 300))
                     v2 = d.height - 300;
                 setLocation(v1, v2); 
                 return;
             }
             if (type.equals("#size")) {
                 setSize(v1, v2); 
                 return;
             }
             if (type.equals("#lw")) {
                 lineWidth = v1;
                 return;
             }
         }
         catch( Exception e ) { }
     }

     private void initOptions() {
         int id, show;
         channelColors = new VColorChooser[channelNum];
         visibleCks = new JCheckBox[channelNum];
         for (id = 0; id < channelNum; id++) {
             channelColors[id]= new VColorChooser(null, null, null);
             channelColors[id].setColor("darkGreen");
             visibleCks[id] = new JCheckBox();
             visibleCks[id].setSelected(true);
             visibleCks[id].setActionCommand("visible");
             visibleCks[id].setToolTipText("Show this channel");
         }
         channelColors[baseLineId].setColor("lightGray");
         channelColors[selectedId].setColor("brown");
         Dimension dim = visibleCks[1].getPreferredSize();
         channelColors[selectedId].setLocation(dim.width+14, 0);
         visibleCks[selectedId].setVisible(false);
         channelColors[durationId].setVisible(false);
         visibleCks[durationId].setToolTipText(null);
 
         rfShapePanel = new JPanel();
         SimpleHLayout layout = new SimpleHLayout();
         // layout.setXOffset(dim.width+14);
         rfShapePanel.setLayout(layout);
         JLabel lb = new JLabel(" RF Shape:  ");
         rfShapePanel.add(lb);
         ButtonGroup butGrp = new ButtonGroup();
         absShapeRadio = new JRadioButton("Absolute", true);
         absShapeRadio.setActionCommand(RFSHAPE);
         absShapeRadio.addActionListener(this);
         absShapeRadio.setOpaque(false);
         butGrp.add(absShapeRadio);
         rfShapePanel.add(absShapeRadio);
         realShapeRadio = new JRadioButton("Real", false);
         realShapeRadio.setActionCommand(RFSHAPE);
         realShapeRadio.addActionListener(this);
         realShapeRadio.setOpaque(false);
         butGrp.add(realShapeRadio);
         rfShapePanel.add(realShapeRadio);

         String path = getOptionFile();
         if (path == null)
             return;
         BufferedReader fin = null;
         String line = null;
         try {
             fin = new BufferedReader( new FileReader( path ));
             while ((line = fin.readLine()) != null) {
                if (line.startsWith("#")) {
                    parseProperty(line);
                    continue;
                }
                StringTokenizer tok = new StringTokenizer(line, " \t\r\n");
                if (!tok.hasMoreTokens())
                    continue;
                String data = tok.nextToken();
                id = Integer.parseInt(data);
                if (!tok.hasMoreTokens())
                    continue;
                if (id < 0 || id >= channelNum)
                    continue;
                data = tok.nextToken();
                show = Integer.parseInt(data);
                if (!tok.hasMoreTokens())
                    continue;
                channelColors[id].setColor(tok.nextToken());
                if (show > 0)
                   visibleCks[id].setSelected(true);
                else
                   visibleCks[id].setSelected(false);
             }
         }
         catch( Exception e ) { }
         finally {
            try {
               if (fin != null)
                   fin.close();
            }
            catch (Exception e2) {}
         }
     }

     private void saveOptions() {
         String path = getOptionFile();
         if (path == null)
             return;
         PrintWriter fout = null;
         try {
             fout = new PrintWriter( new FileWriter( path ));
             Point pt = getLocation();
             Dimension dim = getSize();
             fout.println("#loc "+pt.x+" "+pt.y);
             fout.println("#size "+dim.width+" "+dim.height);
             fout.println("#lw "+lineWidth+" "+lineWidth);
             for (int n = 1; n < channelNum; n++) {
                 if (visibleCks[n].isSelected())
                    fout.print(""+n+" 1 ");
                 else
                    fout.print(""+n+" 0 ");
                 fout.println(channelColors[n].getAttribute(VObjDef.VALUE));
             } 
         }
         catch( Exception e ) { }
         finally {
            try {
               if (fout != null)
                   fout.close();
            }
            catch (Exception e2) {}
         }
     }

     private void closeProc() {
         saveOptions();
     }

     public boolean isChannelVisible(int id) {
         if (id < 0 || id >= channelNum)
            return false;
         return visibleCks[id].isSelected();
     }

     public boolean isBaseLineVisible() {
         return visibleCks[baseLineId].isSelected();
     }

     public boolean isDurationVisible() {
         return visibleCks[durationId].isSelected();
     }

     public Color getChannelColor(int id) {
         if (id < 0 || id >= channelNum)
            return Color.green;
         String str = channelColors[id].getAttribute(VObjDef.VALUE);
         if (str != null)
            return (DisplayOptions.getColor(str));
         return Color.green;
     }

     public Color getBaseLineColor() {
         return getChannelColor(baseLineId);
     }

     public Color getSelectedColor() {
         return getChannelColor(selectedId);
     }

     public int getLineWidth() {
         return lineWidth;
     }

     public boolean isRealRfShape() {
         return (realShapeRadio.isSelected());
     }

     public void actionPerformed(ActionEvent  evt)
     {
         String cmd = evt.getActionCommand();
         if (cmd.equals("close")) {
              closeProc();
              setVisible(false);
              return;
         }
         if (cmd.equals(RFSHAPE)) {
              DpsScopePanel scopePan = DpsUtil.getScopePanel();
              if (scopePan != null)
                  scopePan.setRfShape(realShapeRadio.isSelected());
              return;
         }
     }

     private void lwSelection() {
         String s = lwCb.getSelectedItem().toString();
         int num = 1;
         try {
            num = Integer.parseInt(s);
         }
         catch (NumberFormatException er) { }
         if (num == lineWidth)
             return;
         lineWidth = num;
         DpsScopePanel scopePan = DpsUtil.getScopePanel();
         if (scopePan != null)
             scopePan.setLineWidth(lineWidth);
     }

     public void addChannel(int id, String label) {
         if (id < 0 || id >= channelNum)
             return;
         optPanels[id].setLabel(label);
     }

     public void reset() {
         for (int n = 1; n < baseLineId; n++)
             optPanels[n].setVisible(false);
     }

     public void showChannelOption(int id, boolean bShow) {
         if (id < 0 || id >= channelNum)
             return;
         optPanels[id].setVisible(bShow);
     }

     protected void setChannelColor(int id, String colorName) {
         if (colorName == null)
             return;
         Color c = DisplayOptions.getColor(colorName);
         DpsScopePanel scopePan = DpsUtil.getScopePanel();
         if (scopePan == null)
             return;
         if (id == baseLineId)
             scopePan.setBaseLineColor(c);
         else if (id == selectedId)
             scopePan.setSelectedColor(c);
         else
             scopePan.setChannelColor(id, c);
     }

     protected void setChannelVisible(int id) {
         DpsScopePanel scopePan = DpsUtil.getScopePanel();
         boolean bVisible = visibleCks[id].isSelected();
         if (id == baseLineId) {
             scopePan.setBaseLineVisible(bVisible);
             return;
         }
         if (id == durationId) {
             scopePan.setDurationVisible(bVisible);
             return;
         }
         scopePan.setChannelVisible(id, bVisible);
     }

     private class OptionPanel extends JPanel implements ActionListener {
          protected int  id;
          protected JLabel  chanLabel;

          public OptionPanel(int n) {
              this.id = n;
              SimpleHLayout layout = new SimpleHLayout();
              layout.setVerticalAlignment(SimpleHLayout.CENTER);
              layout.setHgap(10);
              setLayout(layout);
              add(visibleCks[n]);
              channelColors[n].addActionListener(this);
              visibleCks[n].addActionListener(this);
              add(channelColors[n]);
              chanLabel = new JLabel(" channel "+id);
              add(chanLabel);
          }

          public void setLabel(String str) {
              chanLabel.setText(str);
          }

          public void actionPerformed(ActionEvent  evt) {
              String cmd = evt.getActionCommand();
              
              if (cmd.equals("menu") || cmd.equals("button")) { // VColorChooser
                  String value = channelColors[id].getAttribute(VObjDef.VALUE);
                  setChannelColor(id, value);
                  return;
              }
              if (cmd.equals("visible")) {
                  setChannelVisible(id);
                  return;
              }
          }
     }
}
