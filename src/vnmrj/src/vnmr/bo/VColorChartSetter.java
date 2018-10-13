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
import javax.swing.*;
import java.awt.geom.*;
import java.awt.event.*;

import vnmr.ui.*;
import vnmr.util.Util;
import vnmr.util.XVButton;


public class VColorChartSetter extends JPanel implements VColorMapIF {
    private int numColors = 0;
    private int initialIndex = 0;
    private int[][] rgbValues;
    private Color initialColor;
    private JColorChooser colorChooser;
    private boolean bNeedCopy = false;
    private boolean bLoading = false;
    private java.util.List <VColorMapIF> colorListenerList = null;

    public VColorChartSetter()
    {
        buildUi();
        setColorNumber(64);
    }

    private void buildUi() {
        setLayout( new BorderLayout() );
        initialColor = Color.white;
        colorChooser = new JColorChooser(initialColor);
        add(colorChooser, BorderLayout.CENTER);
        FlowLayout layout = new FlowLayout();
        layout.setHgap(20);
        JPanel buttonPane = new JPanel(layout);
        add(buttonPane, BorderLayout.SOUTH);

        String str = Util.getLabel("blApply", "Apply");
        XVButton applyButton = new XVButton(str);
        applyButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                applyColor();
            }
        });

        str = Util.getLabel("blReset", "Reset");
        XVButton resetButton = new XVButton(str);
        resetButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                resetColor();
            }
        });
        buttonPane.add(applyButton);
        buttonPane.add(resetButton);
    }

    public void setColorNumber(int n) {
        if (n < 2 || n == numColors)
           return;
        numColors = n;
        rgbValues = null;
    }

    public void setLoadMode(boolean b) {
        bLoading = b;
    }

    private void updateColor(Color c) {
        if (c == null || colorListenerList == null)
            return;
        synchronized(colorListenerList) {
            Iterator itr = colorListenerList.iterator();
            while (itr.hasNext()) {
                VColorMapIF l = (VColorMapIF)itr.next();
                l.setColor(-1, c);
            }
        }
    }

    private void applyColor() {
        Color c = colorChooser.getColor();
        updateColor(c);
    }

    private void resetColor() {
        colorChooser.setColor(initialColor);
        updateColor(initialColor);
    }

    public void setLookupRgbs(int[] rgbs) {
        if (rgbs == null)
            return;
        int num = rgbs.length;
        if  (num < 2)
            return;
        int k = 0;
        int filter = 0xff;
        int firstIndex = 0;
        if (num > numColors) {
            num = numColors + 1;
            firstIndex = 1;
        }
        if (rgbValues == null)
            rgbValues = new int[3][numColors];
        for (int i = firstIndex; i < num; i++) {
            int v = rgbs[i]; 
            rgbValues[0][k] = (v >> 16) & filter;
            rgbValues[1][k] = (v >> 8) & filter;
            rgbValues[2][k] = v & filter;
            k++;
        }
        if (initialIndex >= 0 && initialIndex < numColors) {
            k = initialIndex; 
            initialColor = new Color(rgbValues[0][k],rgbValues[1][k],
                            rgbValues[2][k]);
            colorChooser.setColor(initialColor);
        }
    }

    private void copyRgbValues() {
        if (colorListenerList == null)
            return;
        int[] rgbs = null;
        synchronized(colorListenerList) {
            Iterator itr = colorListenerList.iterator();
            while (itr.hasNext()) { // usually only one listener, i.e. lookuptable
                VColorMapIF l = (VColorMapIF)itr.next();
                rgbs = l.getRgbs();
                if (rgbs != null)
                    break;
            }
        }
        setLookupRgbs(rgbs);
    }

    private void updateListeners() {
        if (colorListenerList == null || numColors < 2)
            return;
 
        if (bLoading || rgbValues == null)
            return;

        synchronized(colorListenerList) {
            Iterator<VColorMapIF> itr = colorListenerList.iterator();
            while (itr.hasNext()) {
                VColorMapIF l = (VColorMapIF)itr.next();
                l.setRgbs(rgbValues[0], rgbValues[1], rgbValues[2]);
            }
        }
    }


    @Override
    public void setVisible(boolean b) {
        super.setVisible(b);
        if (colorListenerList == null)
            return;
        
        if (!b) {
            if (bNeedCopy)
                copyRgbValues();
            bNeedCopy = false;
            return;
        }
        bNeedCopy = true;
        if (rgbValues == null) {
            copyRgbValues();
            return;
        } 
        updateListeners();
    }

    // VColorMapIF

    public void setRgbs(int index, int[] r, int[] g, int[] b) {
        if (r == null || g == null || b == null)
            return;
        int num = r.length;
        if (num < 2)
            return;
        if (rgbValues == null)
            rgbValues = new int[3][numColors];
        if (num > numColors)
            num = numColors;
        
        for (int i = index; i < num; i++) {
           rgbValues[0][i] = r[i];
           rgbValues[1][i] = g[i];
           rgbValues[2][i] = b[i];
        }
        updateListeners();
    }

    public void setRgbs(int[] r, int[] g, int[] b) {
        setRgbs(0, r, g, b);
    }

    public void setRgbs(int index, byte[] r, byte[] g, byte[] b, byte[] a) {
    }

    public void setRgbs(byte[] r, byte[] g, byte[] b, byte[] a) {
    }

    public void setOneRgb(int index, int r, int g, int b) {
    }

    public void setColorEditable(boolean b) {
    }

    public void setColor(int n, Color c) {
        initialColor = c;
        colorChooser.setColor(c);
        if (n < 0 || n >= numColors)
           return;
        initialIndex = n;
        if (rgbValues == null)
           return;
        rgbValues[0][0] = c.getRed();
        rgbValues[0][1] = c.getGreen();
        rgbValues[0][2] = c.getBlue();
    }

    public int[] getRgbs() {
        return null;
    }
 
    public Color[] getColorArray() {
        return null;
    }

    public void setSelectedIndex(int i) {
    }

    public void addColorEventListener(VColorMapIF l) {
        if (colorListenerList == null)
            colorListenerList = Collections.synchronizedList(new LinkedList<VColorMapIF>());
        if (!colorListenerList.contains(l)) {
            colorListenerList.add(l);
        }
    }

    public void clearColorEventListener() {
        if (colorListenerList != null)
            colorListenerList.clear();
    }

    // end of VColorMapIF
}
