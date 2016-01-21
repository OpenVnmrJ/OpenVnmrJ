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
import javax.swing.event.MouseInputAdapter;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import java.awt.geom.*;
import java.awt.event.*;
import java.awt.image.*;
import java.beans.*;

import vnmr.util.*;
import vnmr.ui.*;

public class VColorLookupPanel extends JPanel implements ActionListener, VColorMapIF {
    private Color[] colorArray;
    private Color belowThreshold;
    private Color aboveThreshold;
    private boolean[] bTransparent;
    private boolean[] bTranslucent;
    private boolean[] bOrigTransparent;
    private boolean[] bOrigTranslucent;
    private boolean bTxtEditable = false;
    private boolean bShowThChooser = true;
    private boolean bLoading = false;
    private boolean bDebugMode = false;
    private boolean bBarOnly = false;
    private JLabel colorLabel;
    private JLabel indexInfo;
    private JLabel redLabel;
    private JLabel grnLabel;
    private JLabel bluLabel;
    private JLabel mapNameLabel;
    private JLabel translucentLabel;
    private ColorBar bar;
    private JPanel titlePan;
    private JPanel infoPan;
    private JTextField redTxt;
    private JTextField grnTxt;
    private JTextField bluTxt;
    private VColorImageViewer imgViewer;
    private JCheckBox transparentCk;
    private JCheckBox translucentCk;
    private JComboBox sizeCb;
    private XVButton imgLoadButton;
    private VColorChooser colorChooser;
    private JSlider transSlider;
    private VColorMapEditor editor;
    private int tableSize;
    private int colorNum;
    private int ptrIndex = 1; // selected id
    private int barHeight = 26;
    private int alphaValue = 255;
    private int opaqueValue = 255;
    private int rgbFilter = 0x00ffffff;
    private int mFontHt;
    private int sliderW, sliderH;
    private int transStrWidth = 0;
    private double translucency; 
    private double origTranslucency; 
    private int[] rgbValues;
    private int[] ArgbValues;
    private int[] origRgbValues;
    private Font mFont;
    private String transStr;
    private java.util.List <VColorMapIF> colorListeners = null;
    private java.util.List <VColorMapListenerIF> translucentListeners = null;
    private String[] sizeNameList = VColorMapFile.SIZE_NAME_LIST;
    private int[] sizeValueList = VColorMapFile.SIZE_VALUE_LIST;
    private VColorModel vcm;
    private IndexColorModel indexCm;

    public VColorLookupPanel(boolean barOnly) {
         this.bBarOnly = barOnly;
         this.tableSize = 0;
         this.colorNum = 0;
         buildUi();
    }

    public VColorLookupPanel() {
         this(false);
    }

    public void setColorMapEditor(VColorMapEditor ed) {
         editor = ed;
    } 

    private Rectangle addInfoItem(int x, int y, JComponent comp) {
         Dimension dim = comp.getPreferredSize(); 

         if (dim.width < 10) {
             dim.width = 40;
             dim.height = 20;
         }
         infoPan.add(comp);
         Rectangle rect = new Rectangle(x, y, dim.width, dim.height);
         comp.setBounds(rect);
         return rect;
    }

    private void buildUi() {
         int x, y, w, h, x2;
         Rectangle r;
         JLabel lb;

         setLayout(new tableLayout());

         titlePan = new JPanel();
         titlePan.setLayout(new SimpleHLayout());
         JLabel titleLabel = new JLabel("Color look-up table: ");
         titleLabel.setFont(new Font("Serif", Font.BOLD, 14));
         titlePan.add(titleLabel);
         sizeCb = new JComboBox();
         for (x = 0; x < sizeNameList.length; x++)
             sizeCb.addItem(sizeNameList[x]);
         if (bBarOnly) {
             mapNameLabel = new JLabel(" ");
             titlePan.add(mapNameLabel);
         }
         else { 
             titlePan.add(sizeCb);
             lb = new JLabel("Colors");
             titlePan.add(lb);
         }

         bar = new ColorBar(bBarOnly);
         add(titlePan);
         add(bar);

         titleLabel.setForeground(Color.blue);

         if (!bBarOnly) {
            imgViewer = new VColorImageViewer();
            add(imgViewer);
         }

         imgLoadButton = new XVButton(Util.getLabel("blImportImage", "Import Image"));
         if (!bBarOnly) {
            imgLoadButton = new XVButton(Util.getLabel("blImportImage", "Import Image"));
            add(imgLoadButton);
            imgLoadButton.setActionCommand("importImage");
            imgLoadButton.addActionListener(this);
         }

         infoPan = new JPanel();
         infoPan.setLayout(new VRubberPanLayout());

         x = 2;
         y = 2;
         w = 400;

         if (!bBarOnly) {
             buildInfoPanel();
             r = translucentCk.getBounds();
             w = r.x + r.width;
             y = r.y + r.height + 4;
         }

         lb = new JLabel("Translucency: ");
         r = addInfoItem(x, y, lb);
         y = r.y + r.height + 4;

         transSlider = new JSlider(SwingConstants.HORIZONTAL,
               0, 100, 0);
         if (!bBarOnly) {
             transSlider.setMinorTickSpacing(5);
             transSlider.setMajorTickSpacing(10);
         }
         else {
             transSlider.setMajorTickSpacing(20);
         }
         transSlider.setPaintTicks(true);
         transSlider.setSnapToTicks(false);
         transSlider.setPaintTrack(true);
         transSlider.setPaintLabels(true);
         Dimension dim = transSlider.getPreferredSize();
         if (dim.height < 10)
             dim.height = 18;
         dim.width = w;
         transSlider.setPreferredSize(dim);
         x = 4;
         r = addInfoItem(x, y, transSlider);
         sliderW = r.width;
         sliderH = r.height;
         h = r.y + r.height + 4;
         infoPan.setPreferredSize(new Dimension(w, h));
         add(infoPan);
         translucency = 0.0;
         transSlider.addChangeListener(new ChangeListener() {
            public void stateChanged(ChangeEvent evt) {
                 setTranslucency();
            }
         });
         mFont = new Font("Serif", Font.BOLD, 14);
         mFontHt = mFont.getSize();
         barHeight = mFontHt + 16;
         if (barHeight < 26)
             barHeight = 26;
         transStr = "Translucency   0%";
         FontMetrics fm = getFontMetrics(mFont);
         transStrWidth = fm.stringWidth(transStr); 
         sizeCb.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                sizeSelection();
            }
         });
    }

    private void buildInfoPanel() {
         int x, y, w, h;
         Rectangle r;
         JLabel lb;

         x = 2;
         y = 2;
         h = 8;
         lb = new JLabel("Color: ");
         r = addInfoItem(x, y, lb);

         indexInfo = new JLabel(" below threshold ");
         x = x + r.width + 8;
         r = addInfoItem(x, y, indexInfo);
         x = x + r.width + 18;
         if (r.height > h) 
             h = r.height;

         colorChooser = new VColorChooser(null, null, "color");
         r = addInfoItem(x, 0, colorChooser);
         if (r.height > h)
             h = r.height + 8;
         y += h;
         x = 4;
         h = 8;

         lb = new JLabel("Red: ");
         r = addInfoItem(x, y, lb);
         x = x + r.width;
         if (r.height > h)
             h = r.height;

         redTxt = new JTextField(3);
         redLabel = new JLabel("  ");
         addInfoItem(x, y, redLabel);
         r = addInfoItem(x, y, redTxt);
         x = x + r.width + 8;
         if (r.height > h)
             h = r.height;

         lb = new JLabel("Green: ");
         r = addInfoItem(x, y, lb);
         x = x + r.width;

         grnTxt = new JTextField(3);
         grnLabel = new JLabel("  ");
         addInfoItem(x, y, grnLabel);
         r = addInfoItem(x, y, grnTxt);
         x = x + r.width + 8;

         lb = new JLabel("Blue: ");
         r = addInfoItem(x, y, lb);
         x = x + r.width;

         bluTxt = new JTextField(3);
         bluLabel = new JLabel("  ");
         addInfoItem(x, y, bluLabel);
         r = addInfoItem(x, y, bluTxt);
         x = x + r.width + 8;

         redTxt.setActionCommand("color");
         grnTxt.setActionCommand("color");
         bluTxt.setActionCommand("color");
         redTxt.addActionListener(this);
         grnTxt.addActionListener(this);
         bluTxt.addActionListener(this);
         colorChooser.addActionListener(this);
         redTxt.setVisible(false);
         grnTxt.setVisible(false);
         bluTxt.setVisible(false);

         lb = new JLabel("Transparent: ");
         r = addInfoItem(x, y, lb);
         x = x + r.width;

         transparentCk = new JCheckBox();
         transparentCk.setActionCommand("transparent");
         transparentCk.addActionListener(this);
         r = addInfoItem(x, y - 2, transparentCk);
         if (r.height > h)
             h = r.height;
         x = x + r.width + 4;

         translucentLabel = new JLabel("Translucent: ");
         r = addInfoItem(x, y, translucentLabel);
         x = x + r.width;

         translucentCk = new JCheckBox();
         translucentCk.setActionCommand("translucent");
         translucentCk.addActionListener(this);
         addInfoItem(x, y - 2, translucentCk);
    }

    public void setDebugMode(boolean b) {
        bDebugMode = b;
        if (imgViewer != null)
            imgViewer.setDebugMode(b);
    }

    private int getSizeNumber() {
        String s = sizeCb.getSelectedItem().toString();
        int num = sizeValueList[0];
        try {
            num = Integer.parseInt(s);
        }
        catch (NumberFormatException er) { }
        return num;
    }

    private void sizeSelection() {
        if (bLoading)
            return;
        int num = getSizeNumber();
        if (editor != null)
            editor.setColorNumber(num); 
    }

    public void setImage(BufferedImage img, IndexColorModel cm, 
            int first, int num) {
         if (imgViewer != null)
             imgViewer.setImage(img, cm, first, num);
    }

    public void setImage(VnmrCanvas.XMap xmap) {
         if (xmap == null)
            return;
         BufferedImage img = xmap.getImage();
         if (imgViewer == null || img == null)
            return;
         Color bg = xmap.getBg();
         if (bg != null)
            imgViewer.setBackground(bg);
         int cmpId = xmap.getColormapId();
         if (cmpId < 0)
            cmpId = 0;
         VColorModelPool pool = VColorModelPool.getInstance();
         VColorModel model = pool.getColorModel(cmpId);
         if (model == null) {
            model = vcm;
            if (model == null)
               return;
         }
         indexCm = model.getModel();
         int num = model.getMapSize();
         int w = xmap.width;
         int h = xmap.height;
         imgViewer.setImage(img, w, h, indexCm, 0, num);
         // ImageColorMapEditor.loadColormap(model.getName(), model.getFilePath());
         changeColor();
    }

    public VColorImageViewer getImageViewer() {
         return imgViewer;
    }

    public int getFirstIndex() {
         if (imgViewer == null)
             return 0;
         return imgViewer.getFirstIndex();
    }

    public int getColorSize() {
         if (imgViewer == null)
             return colorNum;
         return imgViewer.getColorSize();
    }

    public byte[] getRedBytes() {
         if (imgViewer == null)
             return null;
         return imgViewer.getRedBytes();
    }
    
    public byte[] getGreenBytes() {
         if (imgViewer == null)
             return null;
         return imgViewer.getGreenBytes();
    }
    
    public byte[] getBlueBytes() {
         if (imgViewer == null)
             return null;
         return imgViewer.getBlueBytes();
    }
    
    public byte[] getAlphaBytes() {
         if (imgViewer == null)
             return null;
         return imgViewer.getAlphaBytes();
    }

    public Color[] getColorArray() {
         return colorArray;
    }

    public void setSelectedIndex(int i) {
         if (ptrIndex == i)
            return;
         ptrIndex = i;
         showColorInfo(ptrIndex);
         bar.repaint();
    }

    private void changeColor() {
         if (tableSize > 1) {
            indexCm = new IndexColorModel(8, tableSize+2, ArgbValues, 0, true,
                       tableSize, DataBuffer.TYPE_BYTE);
            if (imgViewer != null)
               imgViewer.setColorModel(indexCm);
         }
         bar.repaint();
    }

    public void setupOneColor(int n) {
         int rgb = rgbValues[n];

         if (!bTransparent[n]) {
             if (bTranslucent[n])
                 rgb = rgb | (alphaValue << 24);
             else
                 rgb = rgb | (opaqueValue << 24);
         }
         ArgbValues[n] = rgb;
         colorArray[n] = new Color(rgb, true);
    }

    public void setupColors(int startIndex) {
         int lastIndex = colorArray.length;

         for (int i = startIndex; i < lastIndex; i++)
             setupOneColor(i);
    }
    
    public void setRgbs(int startIndex, byte[] r, byte[] g, byte[] b, byte[] a) {
         if (r == null || colorArray == null)
             return;
         int lastIndex = startIndex + r.length;

         if (lastIndex > colorArray.length)
             lastIndex = colorArray.length;
         int k = 0;
         for (int i = startIndex; i < lastIndex; i++) {
             if (a[i] > 0)
                 bTransparent[i] = false;
             else
                 bTransparent[i] = true;
             rgbValues[i] = (((int)r[k] & 0xff) << 16) |
                            (((int)g[k] & 0xff) << 8)  |
                            ((int)b[k] & 0xff);
             k++;
         }
         setupColors(startIndex);
         showColorInfo(ptrIndex);
         updateListeners();
         changeColor();
    }

    public void setRgbs(byte[] r, byte[] g, byte[] b, byte[] a) {
         setRgbs(1, r, g, b, a);
    }

    public void setRgbs(int startIndex, int[] r, int[] g, int[] b) {
         if (r == null || colorArray == null)
             return;
         int lastIndex = startIndex + r.length;

         if (lastIndex > colorArray.length)
             lastIndex = colorArray.length;
         int k = 0;
         for (int i = startIndex; i < lastIndex; i++) {
             rgbValues[i] = ((r[k] & 0xff) << 16) |
                            ((g[k] & 0xff) << 8)  |
                            (b[k] & 0xff);
             k++;
         }
         setupColors(startIndex);
         showColorInfo(ptrIndex);
         updateListeners();
         changeColor();
    }

    public void setRgbs(int[] r, int[] g, int[] b) {
         setRgbs(1, r, g, b);
    }

    public void setOneRgb(int index, int r, int g, int b) {
         index++;
         if (index < 0 || index >= colorArray.length)
             return;
         rgbValues[index] = ((r & 0xff) << 16) |
                            ((g & 0xff) << 8)  |
                            (b & 0xff);
         setupOneColor(index);
         if (index == ptrIndex)
             showColorInfo(colorArray[ptrIndex]);
         changeColor();
    }

    public int[] getRgbs() {
         return rgbValues;
    }

    public void setColor(int n, Color c) {
         if (c == null)
            return;
         if (n < 0 || n >= colorArray.length)
            n = ptrIndex; 
         rgbValues[n] = c.getRGB() & rgbFilter;
         setupOneColor(n);
         if (n == ptrIndex)
            showColorInfo(colorArray[ptrIndex]);
         changeColor();
    }

    public void setColorNumber(int n) {
         int k, oldSize, copyNum;
         Color defColor = Color.white;

         oldSize = getSizeNumber();
         if (oldSize != n)
            sizeCb.setSelectedItem(Integer.toString(n));
         oldSize = sizeValueList.length;
         if (n == colorNum) {
            if (colorArray != null)
                return;
         }
         colorNum = n;
             
         oldSize = 0;
         if (colorArray != null)
            oldSize = colorArray.length;
         n = n + 2; // add underflow and overflow colors
         if (n == oldSize)
            return;
         tableSize = n; 
         Color[] newColors = new Color[n];
         int[] newRgbs = new int[n];
         boolean[] newTrans = new boolean[n];
         boolean[] newTranslucent = new boolean[n];
         ArgbValues = new int[n+2];
         ArgbValues[n] = 0;
         ArgbValues[n+1] = 0;
         copyNum = oldSize - 1;
         if (copyNum >= n)
             copyNum = n - 1;
         if (copyNum < 0)
             copyNum = 0;
         for (k = 1; k < copyNum; k++) {
             newColors[k] = colorArray[k]; 
             newTrans[k] = bTransparent[k]; 
             newTranslucent[k] = bTranslucent[k]; 
             newRgbs[k] = rgbValues[k]; 
         }
         if (copyNum < n) {
             for (k = copyNum; k < n; k++) {
                 newColors[k] = defColor;
                 newTrans[k] = false;
                 newRgbs[k] = 0x00ffff00;
                 newTrans[k] = false;
                 newTranslucent[k] = true;
             }
         }
         if (oldSize > 2) {  // copy underflow and overflow colors
            newColors[0] = colorArray[0];
            newColors[n-1] = colorArray[oldSize-1];
            newTrans[0] = bTransparent[0];
            newTrans[n-1] = bTransparent[oldSize-1];
            newTranslucent[0] = bTranslucent[0];
            newTranslucent[n-1] = bTranslucent[oldSize-1];
            newRgbs[0] = rgbValues[0]; 
            newRgbs[n-1] = rgbValues[oldSize-1]; 
         }
         else {
            newColors[0] = Color.black;
            newColors[n-1] = defColor;
            newTrans[0] = false;
            newTrans[n-1] = false;
            newTranslucent[0] = true;
            newTranslucent[n-1] = true;
            newRgbs[0] = 0x00;
            newRgbs[n-1] = 0x00ffffff;
         }
         colorArray = newColors;
         bTransparent = newTrans;
         bTranslucent = newTranslucent;
         rgbValues = newRgbs;
         setupColors(0);
         changeColor();
    }

    public int getColorNumber() {
         if (colorArray == null)
             return 0;
         return colorArray.length;
    }

    public void showColorInfo(Color c) {
         if (redTxt == null)
             return;
         redTxt.setText(""+c.getRed());
         redLabel.setText(" "+c.getRed());

         grnTxt.setText("" + c.getGreen());
         grnLabel.setText(" " + c.getGreen());

         bluTxt.setText(""+c.getBlue());
         bluLabel.setText(" "+c.getBlue());
    }

    public void setColorEditable(boolean b) {
         bTxtEditable = b;
    }

    public void showThreshColorChooser(boolean b) {
         bShowThChooser = b;
    }

    private void setTranslucentEnabled(boolean bEnable) {
         translucentLabel.setEnabled(bEnable);
         translucentCk.setEnabled(bEnable);
    }

    public void showColorInfo(int index) {
         if (index < 0 || index >= colorArray.length)
            return;
         if (redTxt == null)
             return;
         boolean bEditable = bTxtEditable;
         boolean bChooser = false;
         Color c = colorArray[index];
         showColorInfo(c);
         if (index < 1 || index >= (tableSize -1)) {
             if (index < 1)
                 indexInfo.setText("below minimum");
             else
                 indexInfo.setText("above maximum");
             bEditable = true;
             bChooser = bShowThChooser;
             colorChooser.setColor(DisplayOptions.colorToRGBString(c));
             colorChooser.repaint();
         }
         else
             indexInfo.setText(" "+index);
         redTxt.setVisible(bEditable);
         grnTxt.setVisible(bEditable);
         bluTxt.setVisible(bEditable);
         redLabel.setVisible(!bEditable);
         grnLabel.setVisible(!bEditable);
         bluLabel.setVisible(!bEditable);
         colorChooser.setVisible(bChooser);

         transparentCk.setSelected(bTransparent[index]);
         translucentCk.setSelected(bTranslucent[index]);
         setTranslucentEnabled(!bTransparent[index]);
    }

    private void colorTextAction() {
         String s;
         int r, g, b;
         Color c = colorArray[ptrIndex];

         r = -1;
         g = -1;
         b = -1;
         try {
            s = redTxt.getText();
            if (s.length() > 0)
               r = Integer.parseInt(s);
            s = grnTxt.getText();
            if (s.length() > 0)
               g = Integer.parseInt(s);
            s = bluTxt.getText();
            if (s.length() > 0)
               b = Integer.parseInt(s);
         }
         catch (NumberFormatException nfe) { }
         if (r < 0)
             r = c.getRed();
         else if (r > 255)
             r = 255;
         if (g < 0)
             g = c.getGreen();
         else if (g > 255)
             g = 255;
         if (b < 0)
             b = c.getBlue();
         else if (b > 255)
             b = 255;
         rgbValues[ptrIndex] = ((r & 0xff) << 16) | ((g & 0xff) << 8)  |
                (b & 0xff);
         setupOneColor(ptrIndex);
         showColorInfo(colorArray[ptrIndex]);
    }

    public double getTranslucency() {
         int v = transSlider.getValue();
         translucency = (double)v / 100.0;
         return translucency;
    }

    private void setTranslucency() {
         getTranslucency();
         double fv = 1.0 - translucency;
         if (fv < 0.0)
             fv = 0.0;
         if (fv > 1.0)
             fv = 1.0;
         fv = 255.0 * fv;
         alphaValue = (int) fv;
         
         int v = transSlider.getValue();
         transStr = "Translucency  "+v+"%";
         setupColors(0);
         changeColor();
         updateTranslucencyListeners();
    }

    private void colorChooserAction() {
         String s = colorChooser.getAttribute(VObjDef.VALUE);
         Color c = DisplayOptions.getColor(s);
         rgbValues[ptrIndex] = c.getRGB() & rgbFilter;
         setupOneColor(ptrIndex);
         showColorInfo(c);
    }

    public void actionPerformed(ActionEvent  evt)
    {
         String cmd = evt.getActionCommand();

         if (cmd.equals("transparent")) {
             bTransparent[ptrIndex] = transparentCk.isSelected();
             setupOneColor(ptrIndex);
             changeColor();
             setTranslucentEnabled(!bTransparent[ptrIndex]);
             return;
         }
         if (cmd.equals("color")) {
             colorTextAction();
             updateListeners();
             changeColor();
             return;
         }
         if (cmd.equals("menu") || cmd.equals("button")) {
             colorChooserAction();
             updateListeners();
             changeColor();
             return;
         }
         if (cmd.equals("translucent")) {
             bTranslucent[ptrIndex] = translucentCk.isSelected();
             setupOneColor(ptrIndex);
             changeColor();
             return;
         }
         if (cmd.equals("importImage")) {
             ExpPanel exp = Util.getActiveView();
             if (exp == null)
                 return;
             Object obj = exp.getAipMap();
             if (obj != null) {            
                 if (obj instanceof VnmrCanvas.XMap) {
                    setImage((VnmrCanvas.XMap) obj);
                 } 
             }
             return;
         }
    }

    public void addColorEventListener(VColorMapIF l) {
        if (colorListeners == null)
            colorListeners = Collections.synchronizedList(new LinkedList<VColorMapIF>());
        if (!colorListeners.contains(l)) {
            colorListeners.add(l);
        }
    }

    public void clearColorEventListener() {
        if (colorListeners != null)
            colorListeners.clear();
    }

    private void updateListeners() {
        if (bLoading || colorListeners == null)
            return;
        int n = ptrIndex - 1;
        if (n < 0 || n >= colorNum) {
            if (bShowThChooser)
                return;
        }
        synchronized(colorListeners) {
            Iterator itr = colorListeners.iterator();
            while (itr.hasNext()) {
                VColorMapIF l = (VColorMapIF)itr.next();
                l.setColor(n, colorArray[ptrIndex]);
            }
        }
    }

    public void addTranslucencyListeners(VColorMapListenerIF listener) {
        if (translucentListeners == null)
           translucentListeners = Collections.synchronizedList(new
                  LinkedList<VColorMapListenerIF>());
        if (!translucentListeners.contains(listener)) {
           translucentListeners.add(listener);
        }
    }

    private void updateTranslucencyListeners() {
        if (bLoading || translucentListeners == null)
            return;
        synchronized(translucentListeners) {
            Iterator itr = translucentListeners.iterator();
            while (itr.hasNext()) {
                VColorMapListenerIF l = (VColorMapListenerIF)itr.next();
                l.updateTranslucency(translucency);
            }
        }
    }

    public void saveColormap(PrintWriter outs) {
        if (rgbValues == null)
            return;
        int i, n, v, len;
        double dv;

        len = rgbValues.length;
        for (i = 0; i < len; i++) {
            outs.print(Integer.toString(i));
            v = rgbValues[i];
            n = (v & 0xff0000) >> 16;  // red
            dv = ((double)n) / 255.0;
            n = (int) ((dv + 0.000045)*10000.0);
            dv = ((double) n) / 10000.0;
            outs.print("  "+Double.toString(dv));

            n = (v & 0xff00) >> 8;  // green
            dv = ((double)n) / 255.0;
            n = (int) ((dv + 0.000045)*10000.0);
            dv = ((double) n) / 10000.0;
            outs.print("  "+Double.toString(dv));

            n = v & 0xff;  // blue
            dv = ((double)n) / 255.0;
            n = (int) ((dv + 0.000045)*10000.0);
            dv = ((double) n) / 10000.0;
            outs.print("  "+Double.toString(dv));

            if (bTransparent[i]) 
               outs.print("  1");
            else
               outs.print("  0");
            if (bTranslucent[i]) 
               outs.println("  1");
            else
               outs.println("  0");
        }
    }

    private boolean addColor(int id, StringTokenizer tok) {
        int  r, g, b, tr;
        double d;

        try {
             if (id >= tableSize)
                 return false;
             if (!tok.hasMoreTokens())
                 return false;
             d = Double.parseDouble(tok.nextToken());
             r = (int) (d * 255.0);
             if (r > 255)
                 r = 255;

             if (!tok.hasMoreTokens())
                 return false;
             d = Double.parseDouble(tok.nextToken());
             g = (int) (d * 255.0);
             if (g > 255)
                 g = 255;

             if (!tok.hasMoreTokens())
                 return false;
             d = Double.parseDouble(tok.nextToken());
             b = (int) (d * 255.0);
             if (b > 255)
                 b = 255;

             rgbValues[id] = ((r & 0xff) << 16) | ((g & 0xff) << 8)  |
                            (b & 0xff);

             tr = 0;
             if (tok.hasMoreTokens())
                 tr = Integer.parseInt(tok.nextToken());
             if (tr != 0)
                  bTransparent[id] = true;
             else 
                  bTransparent[id] = false;

             tr = 1;
             if (tok.hasMoreTokens())
                 tr = Integer.parseInt(tok.nextToken());
             if (tr != 0)
                  bTranslucent[id] = true;
             else 
                  bTranslucent[id] = false;

        }
        catch (NumberFormatException ex) { return false; }
        return true;
    }


    public void setLoadMode(boolean b) {
        bLoading = b;
    }

    public void backupColors() {
        int i, num;

        if (rgbValues == null)
           return;
        num = rgbValues.length;
        if (origRgbValues != null) {
            if (origRgbValues.length != num)
                origRgbValues = null;
        }
        if (origRgbValues == null) {
            origRgbValues = new int[num];
            bOrigTransparent = new boolean[num];
            bOrigTranslucent = new boolean[num];
        }
        for (i = 0; i < num; i++) {
            bOrigTransparent[i] = bTransparent[i];
            bOrigTranslucent[i] = bTranslucent[i];
            origRgbValues[i] = rgbValues[i];
        }
        origTranslucency = translucency;
    }

    public void setColorModel(VColorModel cm, int transValue) {
        if (cm == null)
           return;
        vcm = cm;
        byte[] r = vcm.getRedBytes();
        byte[] g = vcm.getGreenBytes();
        byte[] b = vcm.getBlueBytes();
        boolean[] bTr = vcm.getTransparentValues();
        boolean[] bLucent = vcm.getTranslucentValues();

        if (r == null || g == null || b == null)
            return;
        int num = r.length;
        if (num < 4)
            return;
        bLoading = true;
        setColorNumber(num - 2);

        for (int i = 0; i < num; i++) {
            bTransparent[i] = bTr[i];
            bTranslucent[i] = bLucent[i];
            rgbValues[i] = (((int)r[i] & 0xff) << 16) |
                            (((int)g[i] & 0xff) << 8)  |
                            ((int)b[i] & 0xff);
        }
        setupColors(0);
        transSlider.setValue(transValue);
        changeColor();
        backupColors();
        if (mapNameLabel != null) {
           String name = vcm.getName();
           if (name != null)
              mapNameLabel.setText(name+" ("+colorNum+" colors)");
        }
        bLoading = false;
    }

    public void setColorModel(int id, int transValue) {
        VColorModelPool pool = VColorModelPool.getInstance();
        VColorModel cm = pool.getColorModel(id);
        if (cm == null)
           return;
        setColorModel(cm, transValue);
    }

    public void setColorModel(int id) {
        setColorModel(id, 0);
    }

    public void loadColormap(String name, String path) {
        VColorModelPool pool = VColorModelPool.getInstance();
        vcm = pool.openColorModel(name, path);
        if (vcm == null)
            return;
        translucency = vcm.getTranslucency();
        double trs = translucency * 100.0;
        if (trs < 0.0)
            trs = 0.0; 
        if (trs > 100.0)
            trs = 100.0; 
        setColorModel(vcm, (int) trs);
    }

    public boolean isColorChanged() {
        if (origRgbValues == null)
            return true;
        if (origTranslucency != translucency)
            return true;
        boolean bNewColor = false;
        int num = origRgbValues.length;
        for (int i = 0; i < num; i++) {
            if (bTransparent[i] != bOrigTransparent[i])
                bNewColor = true;
            if (bTranslucent[i] != bOrigTranslucent[i])
                bNewColor = true;
            if (rgbValues[i] != origRgbValues[i])
                bNewColor = true;
            if (bNewColor)
                break;
        }
        return bNewColor;
    }

    private void layoutComponents(int width, int height) {
         Dimension dim = titlePan.getPreferredSize(); 
         int y = 0;
         int x = 0;
         int w = width - 2;
         
         if (w < 10)
             w = 10;
         if (imgViewer != null) {
             imgViewer.setBounds(new Rectangle(0, 0, 130, 130));
             w = width - 132;
             x = 132;
         }
         if (imgLoadButton != null) {
             Dimension dim2 = imgLoadButton.getPreferredSize(); 
             imgLoadButton.setBounds(new Rectangle(2, 132, dim2.width, dim2.height));
         }

         if (dim.height < 2) {
             dim.height = 20;
             dim.width = 220;
         }
         titlePan.setBounds(new Rectangle(x, 0, w, dim.height));
         y += dim.height;
         bar.setBounds(new Rectangle(x, y, w, barHeight));
         y += 30;

         dim = infoPan.getPreferredSize(); 
         if (dim.height < 2) {
             dim.height = 20;
         }
         if (w < sliderW) {
            transSlider.setPreferredSize(new Dimension(w-8, sliderH));
         }
         else
            transSlider.setPreferredSize(new Dimension(sliderW, sliderH));
         infoPan.setBounds(new Rectangle(x, y, w, dim.height));
    }

    private class tableLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {}

        public void removeLayoutComponent(Component comp) {}

        public Dimension preferredLayoutSize(Container target) {
            Dimension dim = titlePan.getPreferredSize(); 
            Dimension dim2 = infoPan.getPreferredSize(); 
            Dimension dim3 = imgLoadButton.getPreferredSize(); 
            dim.height = dim.height + dim2.height + barHeight;
            int minH = dim3.height + 130;
            if (bBarOnly)
                minH = 40;
            if (dim.height < minH)
                dim.height = minH;
            
            dim.width = 400;
            return dim;
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0);
        }

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Dimension panSize = target.getSize();
                layoutComponents(panSize.width, panSize.height);
            }
        }
    }

    private class ColorBar extends JComponent {
        int  specX0 = 0;
        int  specX1 = 0;
        int  specW = 0;
        int  barH = 0;
        int  barX = 0;
        int  barY = 0;
        int  barW = 0;
        double Xgap = 10;
        boolean bViewOnly = false;

        public ColorBar(boolean viewOnly) {
             this.bViewOnly = viewOnly;
             if (!viewOnly)
                addEventHandler();
        }

        private void addEventHandler() {
             addMouseListener(new MouseAdapter() {
                public void mouseClicked(MouseEvent evt) {
                    mouseClickProc(evt); 
                }
                public void mousePressed(MouseEvent evt) {
                }
             });

             KeyboardFocusManager.getCurrentKeyboardFocusManager().addKeyEventDispatcher(
                new KeyEventDispatcher() {
                    public boolean dispatchKeyEvent(KeyEvent e) {
                       if (e.getID() == KeyEvent.KEY_RELEASED) {
                          int mod = e.getModifiers();
                          if (mod != 0)
                              return false;
                          int kcode = e.getKeyCode();
                          int n = ptrIndex;
                          if (kcode == KeyEvent.VK_LEFT) {
                              n--;
                              if (n < 0)
                                  n = tableSize - 1;
                          }
                          if (kcode == KeyEvent.VK_RIGHT) {
                              n++;
                              if (n >= tableSize)
                                  n = 0;
                          }
                          if (n != ptrIndex) {
                              ptrIndex = n;
                              showColorInfo(ptrIndex);
                              updateListeners();
                              repaint();
                          }
                        }
                        return false;
                    }
              });
        }

        public void mouseClickProc(MouseEvent evt) {
             int x = evt.getX();
             int y = evt.getY();
             int selectedIndex = -2;

             if (x >= specX0 && x <= (specX0 + specW))
                 selectedIndex = 0;
             else if (x >= specX1 && x <= (specX1 + specW))
                 selectedIndex = colorNum + 1;
             else if (x >= barX && x <= (barX + barW)) {
                 x = x - barX;
                 double dx = (double) x / Xgap + 1.0;
                 selectedIndex = (int)dx;
             }
             if (selectedIndex >= 0 && (selectedIndex != ptrIndex)) {
                 ptrIndex = selectedIndex;
                 showColorInfo(ptrIndex);
                 updateListeners();
                 repaint();
             }
        }

        public void paint(Graphics g) {
             Dimension dim = getSize();
             int w = dim.width;
             int h = dim.height;
             int i, x, x2, cols, y, y2;
             double dx, dy, dx0;

             if (colorArray == null)
                 return;
             if (colorNum < 2)
                 return;
             barH = h - 14; 
             if (barH < 8)
                 barH = 8;
             specW = w / (colorNum + 2);
             if (specW < 8)
                 specW = 8;
             barW = w - (specW + 6) * 2;
             Xgap = (double)barW / (double)colorNum;
             specX0 = 2;
             barY = 2;
             g.setColor(colorArray[0]);
             g.fillRect(specX0, barY, specW, barH);
             g.setColor(Color.black);
             g.drawRect(specX0, barY, specW, barH);
             barX = specX0 + specW + 4;
             specX1 = barX + barW + 4;
             g.setColor(colorArray[colorNum +1]);
             g.fillRect(specX1, barY, specW, barH);
             g.setColor(Color.black);
             g.drawRect(specX1, barY, specW, barH);
             
             x = (barW - transStrWidth) / 2;
             if (x > 0) {
                g.setFont(mFont);
                g.drawString(transStr, x + barX, mFontHt+1);
             }
             x = barX;
             dx0 = (double) barX;
             cols = colorNum + 1;
             for (i = 1; i < cols; i++) {
                 dx0 = dx0 + Xgap;
                 x2 = (int) dx0;
                 if (x2 > x) {
                    g.setColor(colorArray[i]);
                    g.fillRect(x, barY, x2 - x, barH);
                    x = x2;
                 }
             }
             g.setColor(Color.black);
             g.drawRect(barX - 1, barY, barW + 1, barH);
             y = barY + barH;
             y2 = y + 3;
             x = barX;
             dx0 = (double) barX;
             cols = colorNum;
             for (i = 1; i < cols; i++) {
                 dx0 = dx0 + Xgap;
                 x2 = (int) dx0 - 1;
                 if (x2 > x) {
                    g.drawLine(x2, y, x2, y2);
                    x = x2;
                 }
             }
            
             if (bViewOnly)
                return;
             Graphics2D g2d = (Graphics2D) g;
             g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
             g2d.setColor(Color.red);
             if (ptrIndex < 1)
                 x = specX0 + specW / 2;
             else if (ptrIndex > colorNum)
                 x = specX1 + specW / 2;
             else {
                 dx = (double) (ptrIndex - 1) + 0.5;
                 x = barX + (int) (Xgap * dx);
             }
             dx = (double) x;
             dy = (double) (y + 4);
             GeneralPath p = new GeneralPath(GeneralPath.WIND_EVEN_ODD);
             p.moveTo(dx, dy);
             p.lineTo(dx - 3, dy + 8);
             p.lineTo(dx + 3, dy+ 8);
             p.lineTo(dx, dy);
             p.closePath();
             g2d.fill(p); 
        }
    }
}

