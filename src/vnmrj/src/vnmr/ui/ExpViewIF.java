/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.util.*;
import javax.swing.*;

/**
 * An ExpViewIF object keep the information if ExpPanel.
 * It sets the number of ExpPanel, maximize or minimize ExpPanel.
 *
 */
// public abstract class  ExpViewIF  extends JComponent {
public abstract class  ExpViewIF  extends JLayeredPane {

    public void smallLarge(int num) {};
    public void toggleVpSize(int num) {};
    public void setActiveCanvas(int num) {};
    public void setCanvasArray(int row, int col) {};
    public void initCanvasArray() {};
    public void setTabPanels(Vector<JComponent> v) {};
    public void setViewMargin(int num) {};
    public void setWindowParam(String p) {};
    public void setWindowsParam(String p) {};
    public void updateValue(Vector<?> p) {};
    public void sendToVnmr(String str) {};
    public void sendToVnmr(int viewport, String str) {};
    public void sendToAllVnmr(String str) {};
    public void sendToAllVnmr(String str, int originator) {};
    public void setCanvasNum(int num) {};
    public void setCanvasNum(int num, String opt) {};
    public void setCanvasNum(int num, String opt, String opt2) {};
    public void canvasExit(int num) {};
    public void overlayCanvas(boolean b) {};
    public void setCanvasAvailable(int id, boolean b) {};
    public void setCanvasOverlayMode(String str) {};
    public void setCanvasGraphicsMode(String str) {};
    public void setOverlayDispType(int n) {};
    public void overlaySync(boolean b) {};
    public void expReady(int n) {};
    public void printCanvas(String s1, String s2, String s3, String s4,
                 String s5, String s6, boolean b) {};
    public void finishPrintCanvas(int expId) {};
} // ExpViewIF
