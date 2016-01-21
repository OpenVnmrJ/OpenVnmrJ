/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.awt.*;
import java.util.*;
import java.io.*;
import javax.swing.*;
import javax.swing.event.*;
import java.awt.geom.*;
import java.awt.event.*;
import java.beans.*;

import vnmr.ui.*;
import vnmr.util.*;


public class VColorPicker extends JPanel implements VColorMapIF {
    private int numColors = 0;
    private String cltFile = null;
    private String cltFileExpr = null;
    private String mapName = null;
    private JPanel mainPanel;
    private VColorLineSetter lineSetter;
    private VColorChartSetter chartSetter;
    private VColorPixelSetter pixelSetter;
    private JComboBox pickerCb;
    private VColorMapIF activeSetter;
    private XVButton importButton;
    private JComponent knotPanel;
    private VColorLookupPanel lookupPanel;
    private VColorMapEditor editor;
    private JMenu applyMenu;
    private boolean bLoading = false;
    private boolean bDebugMode = false;
    private String applySelected = "selected";
    private String applyDisplayed = "displayed";
    private String applyAll = "all";
    private String JFUNC = "jFunc(";
    private java.util.List <VColorMapIF> colorListenerList = null;


    public VColorPicker()
    {
        setLayout( new BorderLayout() );
        mainPanel = new JPanel();
        mainPanel.setLayout(new CardLayout());
        add(mainPanel, BorderLayout.CENTER);

        JPanel bottomPanel = new JPanel();
        bottomPanel.setLayout(new SimpleVLayout());
        add(bottomPanel, BorderLayout.SOUTH);

        JPanel ctrlPanel = new JPanel();
        ctrlPanel.setBorder(BorderFactory.createEtchedBorder());
        bottomPanel.add(ctrlPanel);

        lineSetter = new VColorLineSetter();
        mainPanel.add("line", lineSetter);

        pixelSetter = new VColorPixelSetter();
        mainPanel.add("pixel", pixelSetter);

        chartSetter = new VColorChartSetter();
        mainPanel.add("chooser", chartSetter);

        String str = Util.getLabel("blColor_Picker", "Color Picker")+": ";
        JLabel lb = new JLabel(str);
        ctrlPanel.add(lb);
        lb.setLocation(2, 2);
        pickerCb = new JComboBox();
        str = Util.getLabel("blLinear", "Linear");
        pickerCb.addItem(str);
        str = Util.getLabel("blIndividual", "Individual");
        pickerCb.addItem(str);
        str = Util.getLabel("blChart", "Chart");
        pickerCb.addItem(str);
        pickerCb.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                pickerSelection();
            }
        });
        ctrlPanel.add(pickerCb);

        str = Util.getLabel("blImport_from_Linear", "Import from Linear");
        importButton = new XVButton(str);
        importButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                importLineColor(activeSetter);
            }
        });
        importButton.setVisible(false);
        ctrlPanel.add(importButton);

        knotPanel = lineSetter.getKnotChooserPanel();
        if (knotPanel != null)
            ctrlPanel.add(knotPanel);

        pickerCb.setSelectedIndex(0);
        importLineColor(pixelSetter);
        importLineColor(chartSetter);

       /**************
        FlowLayout layout = new FlowLayout();
        layout.setHgap(10);
        JPanel pan = new JPanel(layout);
        pan.setBorder(BorderFactory.createEtchedBorder());
        bottomPanel.add(pan);

        ActionListener al = new ActionListener() {
            public void actionPerformed(ActionEvent ev) {
                buttonAction(ev);
            }
        };

        ActionListener ml = new ActionListener() {
            public void actionPerformed(ActionEvent ev) {
                menuAction(ev);
            }
        };


        XVButton but = new XVButton(Util.getLabel("blFile", "File"));
        but.addActionListener(al);
        but.setActionCommand("file");
        pan.add(but);

        JMenuBar bar = new JMenuBar();
        applyMenu = new JMenu("Apply");
        VButtonBorder border = new VButtonBorder();
        border.setBorderInsets(new Insets(4,8,4,8));
        applyMenu.setBorder(border);

        str = Util.getLabel("blSelected", "Selected");
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

        bar.add(applyMenu);
        pan.add(bar);
    
        but = new XVButton(Util.getLabel("blClose", "Close"));
        but.addActionListener(al);
        but.setActionCommand("close");
        pan.add(but);
       **************/
    }

    public void setColorMapEditor(VColorMapEditor ed) {
        editor = ed;
    }

    public void setColorNumber(int n) {
        if (n < 2 || n == numColors)
           return;
        numColors = n;
        lineSetter.setColorNumber(n);
        pixelSetter.setColorNumber(n);
        chartSetter.setColorNumber(n);
        repaint();
    }

    public int getColorNumber() {
        return numColors;
    }

    public void addColorEventListener(VColorMapIF l) {
        lineSetter.addColorEventListener(l);
        pixelSetter.addColorEventListener(l);
        chartSetter.addColorEventListener(l);
        if (colorListenerList == null)
            colorListenerList = Collections.synchronizedList(new LinkedList<VColorMapIF>());
        if (!colorListenerList.contains(l)) {
            colorListenerList.add(l);
        }
        if (l instanceof VColorLookupPanel)
            lookupPanel = (VColorLookupPanel) l;
    }

    public void setDebugMode(boolean b) {
        bDebugMode = b;
    }

    public void clearColorEventListener() {
        if (colorListenerList != null)
            colorListenerList.clear();
    }

    public void setRedValues(int firstIndex, int num, byte[] v) {
    }

    public void setGreenValues(int firstIndex,int num, byte[] v) {
    }

    public void setBlueValues(int firstIndex, int num, byte[] v) {
    }

    public int[] getRedValues() {
        return null;
    }

    public int[] getGreenValues() {
        return null;
    }

    public int[] getBlueValues() {
        return null;
    }

    public void setRgbs(int index, int[] r, int[] g, int[] b) {
    }

    public void setRgbs(int[] r, int[] g, int[] b) {
    }

    public void setRgbs(int index, byte[] r, byte[] g, byte[] b, byte[] a) {
    }

    public void setRgbs(byte[] r, byte[] g, byte[] b, byte[] a) {
    }

    public void setOneRgb(int index, int r, int g, int b) {
    }

    public int[] getRgbs() {
        return null;
    }

    public void setColorEditable(boolean b) {
        if (colorListenerList == null)
            return;
        // set LookupTable editable
        synchronized(colorListenerList) {
            Iterator itr = colorListenerList.iterator();
            while (itr.hasNext()) {
                VColorMapIF l = (VColorMapIF)itr.next();
                l.setColorEditable(b);
            }
        }
    }

    public void setColor(int i, Color c) {
        activeSetter.setColor(i, c);
    }

    public Color[] getColorArray() {
        return null;
    }

    public void setSelectedIndex(int i) {
    }

    private void importLineColor(VColorMapIF requester) {
        if (requester == null || requester == lineSetter)
            return;
        int[] r = lineSetter.getRedValues();
        int[] g = lineSetter.getGreenValues();
        int[] b = lineSetter.getBlueValues();
        requester.setRgbs(r, g, b);
    }

    private void pickerSelection() {
        int i = pickerCb.getSelectedIndex();
        
        if (lookupPanel != null) {
            if (i == 2)
                lookupPanel.showThreshColorChooser(false);
            else
                lookupPanel.showThreshColorChooser(true);
        }
        if (i == 0) { // linear
            setColorEditable(false);
            chartSetter.setVisible(false);
            pixelSetter.setVisible(false);
            importButton.setVisible(false);
            activeSetter = lineSetter;
            lineSetter.setVisible(true);
            return;
        }
        if (i == 1) { // individual
            setColorEditable(true);
            chartSetter.setVisible(false);
            lineSetter.setVisible(false);
            importButton.setVisible(true);
            activeSetter = pixelSetter;
            pixelSetter.setVisible(true);
            return;
        }
        if (i == 2) {  // color chart
            setColorEditable(true);
            lineSetter.setVisible(false);
            pixelSetter.setVisible(false);
            activeSetter = chartSetter;
            chartSetter.setVisible(true);
            importButton.setVisible(true);
        }
    }

    public void saveColormap(PrintWriter outs) {
        lineSetter.saveKnotsValues(outs);
    }

    public void setLoadMode(boolean b) {
        bLoading = b;
        lineSetter.setLoadMode(b);
        pixelSetter.setLoadMode(b);
        chartSetter.setLoadMode(b);
    }

    public void loadColormap(String name, String path) {
        mapName = name;
        lineSetter.loadKnotValues(path);
    }

    public void loadFromLookup(VColorLookupPanel lookUp) {
        int[] rgbs = lookUp.getRgbs();
        if (rgbs == null)
           return;

        lineSetter.setLookupRgbs(rgbs);
        pixelSetter.setLookupRgbs(rgbs);
        chartSetter.setLookupRgbs(rgbs);
        lineSetter.repaint();
        pixelSetter.repaint();
    }

    public void setLookupPanel(VColorLookupPanel pan) {
        lookupPanel = pan;
    }

    private void buttonAction( ActionEvent e )
    {
        String cmd = e.getActionCommand();
        ImageColorMapEditor editor = ImageColorMapEditor.getInstance();

        if (cmd.equals("file")) {
            editor.openFilePopup();
            return;
        }
        if (cmd.equals("close")) {
            ImageColorMapEditor.close();
            return;
        }

    }

    private void menuAction( ActionEvent e )
    {
        String cmd = e.getActionCommand();
        ImageColorMapEditor editor = ImageColorMapEditor.getInstance();
        if (lookupPanel == null)
            return;
        
        boolean bNewColor = lookupPanel.isColorChanged();
        String name = mapName; 
        File fd = null;
        if (bNewColor) {
            lookupPanel.backupColors();
            name = null;
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
        int aipId = 0;
        name = UtilB.windowsPathToUnix(mapName);
        String mess = new StringBuffer().append(JFUNC).append(VnmrKey.APPLYCMP)
                    .append(", ").append(code).append(", ").append(aipId)
                    .append(", '").append(name).append("')\n").toString();
        exp.sendToVnmr(mess);
    }
}
