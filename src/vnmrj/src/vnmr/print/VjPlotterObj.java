/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.print;

import javax.swing.*;


public class VjPlotterObj  {
    public String deviceName; // Vnmr plotter
    public String use; // Printer, Plotter, Both
    public String type; // defined in devicetable
    public String host; // computer host name
    public String port; // parallel, /dev/ttya 
    public String baud; // 9600, ...
    public String shared; // Yes, No
    public String printerName;
    public String new_host;
    public String new_port;
    public boolean bSystem;
    public boolean bChanged;
    public boolean bNewSet;
    public boolean bRemoved;
    public boolean bChangeable;
    public boolean bSelected;
    public boolean bExtra;
    public JRadioButton plotterCheck;
    public JRadioButton printerCheck;
    public VjPageAttributes attrSet;

    public VjPlotterObj(String name, boolean isSystem, boolean bAdmin) {
        this.printerName = "";
        this.type = "";
        this.host = "";
        this.port = "";
        this.use = "Both";
        this.bSystem = isSystem;
        this.bChanged = false;
        this.bNewSet = false;
        this.bRemoved = false;
        this.bChangeable = false;
        this.bExtra = false;
        if (name == null)
            this.deviceName = "";
        else
            this.deviceName = name;
        if (!bAdmin) {
            plotterCheck = new JRadioButton();
            printerCheck = new JRadioButton();
        }
    }

    public VjPlotterObj(String name) {
        this(name, false, false);
    }

    public void setName(String name) {
        deviceName = name;
    }

    public void setAttributeSet(VjPageAttributes attr) {
        attrSet = attr;
    }

    public VjPageAttributes getAttributeSet() {
        return attrSet;
    }

    public void setPlotterSelected(boolean b ) {
        if (plotterCheck != null)
            plotterCheck.setSelected(b);
    }

    public void setPrinterSelected(boolean b ) {
        if (printerCheck != null) {
            printerCheck.setSelected(b);
        }
    }

    /***
    public void setSelected(boolean b ) {
        if (printerCheck != null)
            printerCheck.setSelected(b);
    }
    ***/

    public boolean isPrinterSelected() {
        if (printerCheck != null)
           return printerCheck.isSelected();
        return false;
    }

    public boolean isPlotterSelected() {
        if (plotterCheck != null)
           return plotterCheck.isSelected();
        return false;
    }

    /***
    public boolean isSelected() {
        if (printerCheck != null)
           return printerCheck.isSelected();
        return false;
    }
    ****/

    public void dispose() {
        printerCheck = null;
        plotterCheck = null;
        attrSet = null;
    }

} // end of VjPlotterObj

