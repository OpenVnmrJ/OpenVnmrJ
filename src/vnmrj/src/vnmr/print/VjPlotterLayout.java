/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.print;

import java.awt.*;
import java.awt.event.*;
import java.util.*;
import javax.swing.*;
import javax.swing.border.*;
import javax.print.attribute.standard.*;

import vnmr.util.SimpleVLayout;
import vnmr.util.SimpleHLayout;
import vnmr.util.SimpleH2Layout;
import vnmr.util.Util;

public class VjPlotterLayout extends JPanel {
    private JPanel    cmdPan;
    private PagePanel pagePan;
    private JComboBox pageCb, formatCb;
    private JComboBox colorCb;
    private JLabel deviceLabel;
    private JLabel pageLabel, formatLabel;
    private JLabel colorLabel;
    private JLabel dicomHostLabel, dicomPortLabel;
    private JLabel paperWidthLabel, paperHeightLabel;
    private JLabel paperWidthUnit, paperHeightUnit;
    private JLabel resolutionLabel, resolutionUnit;
    private JLabel dicomHostTitle, dicomPortTitle;
    private JLabel lineWidthLabel;
    private JButton resetButton;
    private JTextField leftMarginTxt, rightMarginTxt;
    private JTextField topMarginTxt, bottomMarginTxt;
    private JTextField resolutionTxt;
    private JTextField paperWidthTxt;
    private JTextField paperHeightTxt;
    private JTextField wcmaxTxt, wc2maxTxt;
    private JTextField dicomHostTxt, dicomPortTxt;
    private JTextField lineWidthTxt;
    private ImgRadioButton rbPortrait, rbLandscape;
    private String deviceName;
    private String emptyStr = "";
    private String monoVal = "Mono";
    private String colorVal = "Color";
    private ControlPanel ctrlPan;
    private VjPlotterObj plotDevice;
    private String unitsStr = " ("+Util.getLabel("_mm", "mm")+")";
    private String dpiStr = " ("+Util.getLabel("_dots_per_inch", "dots per inch")+")";
    private String nochangeStr = "  ( "+Util.getLabel("_no_permission_to_change", "no permission to change")+" )";
    protected VjPageAttributes attrSet;
    protected boolean bEditable;
    protected boolean bChangeable;
    protected boolean bNoneDevice;
    protected boolean bViewOnly;
    protected boolean bAdmin;
    protected boolean bDebug;

    public VjPlotterLayout(boolean adm, boolean viewOnly, VjPlotterConfig cf) {
        setLayout(new BorderLayout());
        setBorder(new EtchedBorder(EtchedBorder.LOWERED));
        this.bAdmin = adm;
        this.bViewOnly = viewOnly;
        if (viewOnly)
           bEditable = false;
        else
           bEditable = true;
        bDebug = VjPlotterConfig.debugMode();
        build();
    }

    private void build() {
        pagePan = new PagePanel();
        add(pagePan, BorderLayout.CENTER);
        cmdPan = new JPanel();
        add(cmdPan, BorderLayout.SOUTH);
        cmdPan.setLayout(new BorderLayout());
        ctrlPan = new ControlPanel();

        // cmdPan.add(ctrlPan, BorderLayout.CENTER);
        JScrollPane scrollPane = new JScrollPane(ctrlPan,
                            JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                            JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
        cmdPan.add(scrollPane, BorderLayout.CENTER);
 
        ButtonPanel butPan = new ButtonPanel();
        if (!bViewOnly)
           cmdPan.add(butPan, BorderLayout.SOUTH);
        deviceName = "none";
    }

    public void setEditable(boolean s) {
        boolean bChanged = false;
        if (bViewOnly)
            s = false;
        if (bEditable != s)
            bChanged = true;
        bEditable = s; 
        if (attrSet != null) {
            if (bEditable)
                bChangeable = attrSet.bChangeable;
            else
                bChangeable = false;
            if (bChanged) {
                pagePan.setPageAttribute();
                updateInfo();
            }
        }
        resetButton.setEnabled(bChangeable);
    }

    public void setPageAttribute(VjPageAttributes a) {
        saveInfo();
        if (a == attrSet)
            return;
        attrSet = a;
        if (a != null) {
            a.init();
            if (bEditable)
                bChangeable = a.bChangeable;
            else
                bChangeable = false;
        }
        else {
            bChangeable = false;
        }
        pagePan.setPageAttribute();
        resetButton.setEnabled(bChangeable);
        updateInfo();
    }

    public void setPlotterDevice(VjPlotterObj obj) {
        if (obj != null)
            deviceName = obj.deviceName;
        else
            deviceName = "";
        if (bDebug)
            System.out.println("set plotter: "+deviceName);
        if (obj == plotDevice) {
            if (bDebug)
                System.out.println(" same plotter: "+deviceName);
            return;
        }
        if (obj != null) {
            setPageAttribute(obj.getAttributeSet());
        }
        else {
            setPageAttribute(null);
        }
        plotDevice = obj;
        setDicomInfo();
        setDicomEnabled(bChangeable);
    }

    public void saveInfo() {
        if (attrSet != null)
            ctrlPan.saveInfo();
        saveDicomInfo();
    }

    public void updateInfo() {
        if (deviceName.equals(VjPlotterTable.noneDevice))
           bNoneDevice = true;
        else
           bNoneDevice = false;
        if (bDebug) {
           System.out.println("updateInfo deviceName: "+deviceName);
           if (attrSet != null)
               System.out.println(" type: "+attrSet.typeName);
        }
        ctrlPan.updateInfo();
        pagePan.updateInfo();
        // checkSpectrumSize();
    }

    public void changeRaster(int raster) {
        if (attrSet == null)
           return;
        if (bDebug)
           System.out.println(attrSet.typeName+" change raster: "+ raster);
        attrSet.setRaster(raster);
        updateInfo();
    }

    private void updateLayout() {
        if (attrSet == null)
           return;
        updateInfo(); 
        // pagePan.updateInfo();
        // checkSpectrumSize();
    }

    /*********
    private void checkSpectrumSize() {
        if (attrSet == null)
            return;
        if (bNoneDevice)
            return;
        if (attrSet.drawWidth < attrSet.wcmaxmax)
            wcmaxTxt.setForeground(Color.red);
        else
            wcmaxTxt.setForeground(Color.black);
            
        if (attrSet.drawHeight < attrSet.wc2maxmax)
            wc2maxTxt.setForeground(Color.red);
        else
            wc2maxTxt.setForeground(Color.black);
    }
    **********/

    private void setDicomEnabled(boolean b) {
        if (attrSet == null)
            b = false;
        if (b) {
            getAttr(VjPlotDef.FORMAT);
            // if (!VjPageAttributes.formatList[VjPageAttributes.DICOM].equals(f))
                b = false;
        } 
        dicomHostTitle.setEnabled(b);
        dicomPortTitle.setEnabled(b);
        dicomHostTxt.setEnabled(b);
        dicomPortTxt.setEnabled(b);
        dicomHostTxt.setVisible(b);
        dicomPortTxt.setVisible(b);
        dicomPortLabel.setVisible(!b);
        dicomHostLabel.setVisible(!b);
        dicomPortLabel.setEnabled(b);
        dicomHostLabel.setEnabled(b);
    }

    private void setDicomInfo() {
        dicomHostTxt.setText(emptyStr);
        dicomPortTxt.setText(emptyStr);
        dicomPortLabel.setText(emptyStr);
        dicomHostLabel.setText(emptyStr);
        if (plotDevice == null)
            return;
        String str = plotDevice.new_port;
        if (str == null || str.length() < 1)
            str = plotDevice.port;
        if (str != null) {
            dicomPortLabel.setText(str);
            dicomPortTxt.setText(str);
        }
        str = plotDevice.new_host;
        if (str == null || str.length() < 1)
            str = plotDevice.host;
        if (str != null) {
            dicomHostLabel.setText(str);
            dicomHostTxt.setText(str);
        }
    }

    private void saveDicomInfo() {
        if (!bChangeable || plotDevice == null)
            return;
        plotDevice.new_port = dicomPortTxt.getText().trim();
        plotDevice.new_host = dicomHostTxt.getText().trim();
        if (!plotDevice.new_port.equals(plotDevice.port))
            plotDevice.bChanged = true;
        else if (!plotDevice.new_host.equals(plotDevice.host))
            plotDevice.bChanged = true;
    }

    private void setPaperGeomEditable(boolean b, boolean bLabel) {
        paperWidthTxt.setVisible(b);
        paperHeightTxt.setVisible(b);
        paperWidthUnit.setVisible(b);
        paperHeightUnit.setVisible(b);
        paperWidthLabel.setVisible(bLabel);
        paperHeightLabel.setVisible(bLabel);
    }

    private void setPaperGeom() {
        paperWidthTxt.setText(getAttr(VjPlotDef.PAPER_WIDTH));
        paperWidthLabel.setText(getAttr(VjPlotDef.PAPER_WIDTH)+unitsStr);
        paperHeightTxt.setText(getAttr(VjPlotDef.PAPER_HEIGHT));
        paperHeightLabel.setText(getAttr(VjPlotDef.PAPER_HEIGHT)+unitsStr);
    }

    private void changePaper() {
        if (attrSet == null)
            return;
        VjMediaSizeObj obj = (VjMediaSizeObj)pageCb.getSelectedItem();
        if (obj == null)
            return;
        String paper = obj.getMediaName();
        attrSet.setNewPaper(obj);
        pageLabel.setText(obj.toString());
        if (paper.equals(VjPageAttributes.customStr)) {
             setPaperGeomEditable(bChangeable, !bChangeable);
             return;
        }
        setPaperGeom();
        setPaperGeomEditable(false, true);
        updateInfo();
    }

    private void changeFormat() {
        if (attrSet == null)
            return;
        String f = (String) formatCb.getSelectedItem();
        attrSet.new_formatStr = f;
        formatLabel.setText(f);
        attrSet.actualWidth = attrSet.paperWidth;
        attrSet.actualHeight = attrSet.paperHeight;
        setDicomEnabled(true);
        int raster = 0; // hpgl
        if (f.equals(VjPageAttributes.formatList[VjPageAttributes.HPGL])) {
            rbPortrait.setEnabled(false);
            rbLandscape.setEnabled(false);
            // attrSet.setRaster(raster);
            changeRaster(raster);
            return;
        }
        if (f.equals(VjPageAttributes.formatList[VjPageAttributes.PCL])) {
            if (rbPortrait.isSelected())
                raster = VjPageAttributes.PCL_Portrait;
            else
                raster = VjPageAttributes.PCL_Landscape;
        }
        else {
            if (rbPortrait.isSelected())
                raster = VjPageAttributes.PS_Portrait;
            else
                raster = VjPageAttributes.PS_Landscape;
        }
        rbPortrait.setEnabled(bChangeable);
        rbLandscape.setEnabled(bChangeable);
        // attrSet.setRaster(raster);
        changeRaster(raster);
    }

    private void changeColor() {
        if (attrSet == null)
            return;
        ColorItem obj = (ColorItem) colorCb.getSelectedItem();
        if (obj == null)
            return;
        attrSet.new_formatStr = obj.getName();
        colorLabel.setText(obj.toString());
    }

    protected void addToGB(Component comp, Container cont,
               GridBagLayout gridbag, GridBagConstraints constraints)
    {
        gridbag.setConstraints(comp, constraints);
        cont.add(comp);
    }

    private String getAttr(int index) {
        if (attrSet == null)
            return emptyStr;
        String str = VjPageAttributes.getAttr(attrSet, index);
        if (str == null)
            return emptyStr;
        return str;
    }

    private class ControlPanel extends JPanel {
        MediaPanel pnlMedia;
        OrientationPanel pnlOrientation;
        MarginsPanel pnlMargins;
        WcmaxPanel pnlWcmax;

        public ControlPanel() {
            setBorder(new EtchedBorder(EtchedBorder.LOWERED));
            // GridBagLayout gridbag = new GridBagLayout();
            // GridBagConstraints c = new GridBagConstraints();
            // setLayout(gridbag);
            setLayout(new SimpleVLayout());

            pnlMedia = new MediaPanel();
            add(pnlMedia);

            JPanel pan = new JPanel();

            pan.setLayout(new SimpleHLayout());
            add(pan);

            pnlOrientation = new OrientationPanel();
            pan.add(pnlOrientation);

            pnlMargins = new MarginsPanel();
            pan.add(pnlMargins);

            pnlWcmax = new WcmaxPanel();
            pan.add(pnlWcmax);
        }

        public void updateInfo() {
            pnlMedia.updateInfo();
            pnlOrientation.updateInfo();
            pnlMargins.updateInfo();
            pnlWcmax.updateInfo();
        }

        public void saveInfo() {
            if (attrSet == null || !attrSet.bChangeable)
                return;
            if (bDebug)
               System.out.println(" saveInfo: "+attrSet.typeName);
            attrSet.bChanged = false;
            pnlMedia.saveInfo();
            pnlOrientation.saveInfo();
            pnlMargins.saveInfo();
            pnlWcmax.saveInfo();
        }
    }


    private class MediaPanel extends JPanel implements ActionListener, ItemListener, FocusListener {
        private Dimension labelDim;
        private boolean bChanged;
        private boolean bNewValue;
        private boolean bUpdating;
        private int   labelW = 0;
        private int   labelH = 0;
        private int   n;
        private Vector<JComponent> labelList = new Vector<JComponent>();
        private Vector<JComponent> list2 = new Vector<JComponent>();
        private Vector<JComponent> list3 = new Vector<JComponent>();

        public MediaPanel() {
            super();

            setLayout(new SimpleVLayout());
            JPanel pan0 = new JPanel();
            SimpleHLayout layout = new SimpleHLayout();
            layout.setExtendLast(true);
            pan0.setLayout(layout);
            String lblStr = Util.getLabel("_Device_Name", "Device Name")+":";
            JLabel lblName = new JLabel(lblStr);
            pan0.add(lblName);
            labelList.add(lblName);
            deviceLabel = new JLabel("     ");
            pan0.add(deviceLabel);

            JPanel pan1 = new JPanel();
            layout = new SimpleHLayout();
            layout.setExtendLast(true);
            layout.setVerticalAlignment(SimpleHLayout.CENTER);
            pan1.setLayout(layout);

            lblStr = Util.getLabel("_Paper_Size", "Paper Size")+":";
            lblName = new JLabel(lblStr);
            labelDim = lblName.getPreferredSize();
            if (labelDim != null) {
                if (labelDim.width > labelW)
                    labelW = labelDim.width;
                labelH = labelDim.height;
            }
            pan1.add(lblName);
            labelList.add(lblName);
            // pageCb = new JComboBox(PrinterPaper.getPaperNames());
            pageCb = new JComboBox();
            MediaSize mediaSizes[] = VjPaperMedia.paperMedium;
            String mediaNames[] = VjPaperMedia.paperNames;
            VjMediaSizeObj mobj;
            for (n = 0; n < mediaSizes.length; n++) {
                MediaSize ms = mediaSizes[n];
                if (ms != null) {
                    String name = VjPrintUtil.getMediaResource(
                        ms.getMediaSizeName().toString(), mediaNames[n]);
                    mobj = new VjMediaSizeObj(ms, name);
                    pageCb.addItem(mobj);
                }
            }

            mobj = new VjMediaSizeObj(null, VjPageAttributes.customStr);
            pageCb.addItem(mobj);
            if (mediaSizes.length > 8) {
                if (pageCb.getMaximumRowCount() < 9)
                    pageCb.setMaximumRowCount(9);
            }
                
            pageCb.addItemListener(this);
            pan1.add(pageCb);
            list2.add(pageCb);
            pageCb.setSelectedIndex(0);
            pageLabel = new JLabel("  ");
            pageLabel.setVisible(false);
            pan1.add(pageLabel);
            labelDim = pageCb.getPreferredSize();
            labelDim.width+= 4;
            pageCb.setPreferredSize(labelDim);
            pageLabel.setPreferredSize(labelDim);

            lblStr = " "+Util.getLabel("_Width", "Width")+": ";
            lblName = new JLabel(lblStr);
            lblName.setHorizontalAlignment(SwingConstants.TRAILING);
            pan1.add(lblName);
            list3.add(lblName);
            paperWidthTxt = new JTextField("", 6);
            paperWidthTxt.addActionListener(this);
            paperWidthTxt.addFocusListener(this);
           
            pan1.add(paperWidthTxt);
            paperWidthLabel = new JLabel("  ");
            paperWidthLabel.setVisible(false);
            pan1.add(paperWidthLabel);
            Font ft = paperWidthTxt.getFont();
            paperWidthUnit = new JLabel(unitsStr);
            paperWidthUnit.setFont(ft);
            pan1.add(paperWidthUnit);

            lblStr = Util.getLabel("_Height", "Height")+": ";
            lblName = new JLabel(lblStr);
            pan1.add(lblName);
            paperHeightTxt = new JTextField("", 6);
            paperHeightTxt.addActionListener(this);
            paperHeightTxt.addFocusListener(this);
            pan1.add(paperHeightTxt);
            paperHeightLabel = new JLabel("  ");
            paperHeightLabel.setVisible(false);
            pan1.add(paperHeightLabel);
            paperHeightUnit = new JLabel(unitsStr);
            paperHeightUnit.setFont(ft);
            pan1.add(paperHeightUnit);

            JPanel pan2 = new JPanel();
            layout = new SimpleHLayout();
            layout.setVerticalAlignment(SimpleHLayout.CENTER);
            pan2.setLayout(layout);
            lblStr = Util.getLabel("_Output_Format", "Output Format")+": ";
            lblName = new JLabel(lblStr);
            pan2.add(lblName);
            labelDim = lblName.getPreferredSize();
            if (labelDim != null) {
                if (labelDim.width > labelW)
                    labelW = labelDim.width;
            }
            labelList.add(lblName);
            formatCb = new JComboBox(VjPageAttributes.formatList);
            formatCb.addItemListener(this);
            pan2.add(formatCb);
            list2.add(formatCb);
            formatLabel = new JLabel("   ");
            formatLabel.setVisible(false);
            pan2.add(formatLabel);
            labelDim = formatCb.getPreferredSize();
            labelDim.width += 4;
            formatCb.setPreferredSize(labelDim);
            formatLabel.setPreferredSize(labelDim);

            lblStr = " "+Util.getLabel("_Resolution", "Resolution")+": ";
            lblName = new JLabel(lblStr);
            lblName.setHorizontalAlignment(SwingConstants.TRAILING);
            pan2.add(lblName);
            list3.add(lblName);
            resolutionTxt = new JTextField("", 6);
            resolutionTxt.addFocusListener(this);
            pan2.add(resolutionTxt);
            resolutionLabel = new JLabel("  ");
            resolutionLabel.setVisible(false);
            pan2.add(resolutionLabel);
            lblStr = " ("+Util.getLabel("_dots_per_inch", "dots per inch")+")";
            resolutionUnit = new JLabel(dpiStr);
            resolutionUnit.setFont(ft);
            pan2.add(resolutionUnit);

            JPanel pan3 = new JPanel();
            layout = new SimpleHLayout();
            layout.setVerticalAlignment(SimpleHLayout.CENTER);
            pan3.setLayout(layout);
            lblStr = Util.getLabel("_Color", "Color")+": ";
            lblName = new JLabel(lblStr);
            pan3.add(lblName);
            labelDim = lblName.getPreferredSize();
            if (labelDim != null) {
                if (labelDim.width > labelW)
                    labelW = labelDim.width;
            }
            // lblName.setHorizontalAlignment(SwingConstants.TRAILING);
            labelList.add(lblName);
            colorCb = new JComboBox();
            monoVal = Util.getLabel("_Monochrome", "Mono");
            ColorItem cobj = new ColorItem(monoVal, "Mono");
            colorCb.addItem(cobj);
            colorVal = Util.getLabel("_Color", "Color");
            cobj = new ColorItem(colorVal, "Color");
            colorCb.addItem(cobj);
            colorCb.addItemListener(this);
            pan3.add(colorCb);
            list2.add(colorCb);
            colorLabel = new JLabel("    ");
            colorLabel.setVisible(false);
            pan3.add(colorLabel);
            labelDim = colorCb.getPreferredSize();
            labelDim.width += 4;
            colorCb.setPreferredSize(labelDim);
            colorLabel.setPreferredSize(labelDim);

            lblStr = " "+Util.getLabel("_Graphics_Line_Width", "Line Width")+": ";
            lblName = new JLabel(lblStr);
            lblName.setHorizontalAlignment(SwingConstants.TRAILING);
            pan3.add(lblName);
            list3.add(lblName);
            lineWidthTxt = new JTextField("1", 4);
            lineWidthTxt.addFocusListener(this);
            pan3.add(lineWidthTxt);
            lineWidthLabel = new JLabel(" 1");
            lineWidthLabel.setVisible(false);
            pan3.add(lineWidthLabel);

            JPanel dicomPan = new JPanel();
            layout = new SimpleHLayout();
            layout.setVerticalAlignment(SimpleHLayout.CENTER);
            dicomPan.setLayout(layout);
            lblStr = Util.getLabel("_Dicom_Printer_Host", "Dicom Printer Host")+": ";
            dicomHostTitle = new JLabel(lblStr);
            dicomPan.add(dicomHostTitle);
            labelDim = lblName.getPreferredSize();
            if (labelDim != null) {
                if (labelDim.width > labelW)
                    labelW = labelDim.width;
            }
            labelList.add(lblName);
            dicomHostTxt = new JTextField("", 12);
            dicomPan.add(dicomHostTxt);
            dicomHostLabel = new JLabel("      ");
            dicomHostLabel.setVisible(false);
            dicomPan.add(dicomHostLabel);
            lblStr = Util.getLabel("_Dicom_Printer_Port", "Dicom Printer Port")+": ";
            dicomPortTitle = new JLabel(lblStr);
            dicomPan.add(dicomPortTitle);
            dicomPortTxt = new JTextField("", 6);
            dicomPortLabel = new JLabel("   ");
            dicomPortLabel.setVisible(false);
            dicomPan.add(dicomPortTxt);
            dicomPan.add(dicomPortLabel);
            labelDim = dicomPortTxt.getPreferredSize();
            dicomPortLabel.setPreferredSize(labelDim);
            setDicomEnabled(false);

            labelW = 0;
            labelH = 0;
            Component comp;
            Dimension dim1;
            for (n = 0; n < labelList.size(); n++) {
                comp = labelList.elementAt(n);
                dim1 = comp.getPreferredSize();
                if (dim1.width > labelW)
                     labelW = dim1.width;
                if (dim1.height > labelH)
                     labelH = dim1.height;
            }
            if (labelW < 10) {
                labelW = 100;
                labelH = 18;
            }
            dim1 = new Dimension(labelW+4, labelH);
            for (n = 0; n < labelList.size(); n++) {
                comp = labelList.elementAt(n);
                comp.setPreferredSize(dim1);
            }
            int tw = 0;
            int w1 = 0;
            int w2 = 0;
            for (n = 0; n < list2.size(); n++) {
                comp = list2.elementAt(n);
                w1 = 0;
                w2 = 0;
                if (comp != null)
                    w1 = comp.getPreferredSize().width;
                comp = list3.elementAt(n);
                if (comp != null)
                    w2 = comp.getPreferredSize().width;
                if ((w1 + w2) > tw)
                     tw = w2 + w2;
            }
 
            if (tw > 2) {
                for (n = 0; n < list2.size(); n++) {
                     comp = list2.elementAt(n);
                     if (comp != null)
                        w1 = comp.getPreferredSize().width;
                     comp = list3.elementAt(n);
                     if (comp != null) {
                        dim1 = comp.getPreferredSize();
                        w2 = tw - w1 - dim1.width;
                        if (w2 > 2) {
                           dim1.width = dim1.width + w2;
                           comp.setPreferredSize(dim1);
                        }
                     }
                }
            }

            add(pan0);
            add(pan1);
            add(pan2);
            add(pan3);
            // add(dicomPan);
        }
 
        private void setPaperInfo() {
            setPaperGeom();
            resolutionTxt.setText(getAttr(VjPlotDef.RESOLUTION));
            resolutionLabel.setText(getAttr(VjPlotDef.RESOLUTION)+dpiStr);
            String str = getAttr(VjPlotDef.FORMAT);
            VjPrintUtil.setComboxSelectItem(formatCb, str);
            formatLabel.setText(str);
            if (bDebug)
               System.out.println("   format: "+str);
            str = getAttr(VjPlotDef.PAPER);
            if (bDebug)
               System.out.println("   paper: "+str);
            VjMediaSizeObj md = VjPrintUtil.setMediaComboxSelectItem(pageCb, str);
            if (md == null) {
                 str = VjPageAttributes.customStr;
                 md = VjPrintUtil.setMediaComboxSelectItem(pageCb, str);
            }
            attrSet.new_paperSize = str;

            str = getAttr(VjPlotDef.COLOR);
            if (bDebug)
               System.out.println("   color: "+str);
            int n1 = 0;
            if (str != null) {
               if (str.equalsIgnoreCase("color"))
                  n1 = 1;
            }
            colorCb.setSelectedIndex(n1);
            if (n1 == 0)
               colorLabel.setText(monoVal);
            else
               colorLabel.setText(colorVal);

            str = getAttr(VjPlotDef.LINEWIDTH);
            if (str != null) {
               lineWidthLabel.setText(str);
               lineWidthTxt.setText(str);
            }

            boolean bTrueDevice = true;
            if (bNoneDevice) {
                 // bTrueDevice = false;
                 // pageLabel.setText(emptyStr);
                 // formatLabel.setText(emptyStr);
                 // resolutionLabel.setText(emptyStr);
                 // colorLabel.setText(emptyStr);
            }
            else {
                 if (md != null)
                    pageLabel.setText(md.toString());
                 else
                    pageLabel.setText(attrSet.new_paperSize);
            }
            if (attrSet.new_paperSize.equals(VjPageAttributes.customStr)) {
                 setPaperGeomEditable(bChangeable, !bChangeable);
            }
            else {
                 setPaperGeomEditable(false, bTrueDevice);
            }
        }

        public void updateInfo() {
            bUpdating = true;
            if (attrSet != null) {
                if (bEditable && !attrSet.bChangeable)
                    deviceLabel.setText(deviceName+nochangeStr);
                else
                    deviceLabel.setText(deviceName);
                setPaperInfo();
            }
            else
                deviceLabel.setText(deviceName);
            pageCb.setVisible(bChangeable);
            formatCb.setVisible(bChangeable);
            pageCb.setEnabled(bChangeable);
            formatCb.setEnabled(bChangeable);
            pageLabel.setVisible(!bChangeable);
            formatLabel.setVisible(!bChangeable);
            resolutionTxt.setVisible(bChangeable);
            resolutionUnit.setVisible(bChangeable);
            resolutionLabel.setVisible(!bChangeable);
            colorCb.setVisible(bChangeable);
            colorCb.setEnabled(bChangeable);
            colorLabel.setVisible(!bChangeable);
            lineWidthTxt.setVisible(bChangeable);
            lineWidthLabel.setVisible(!bChangeable);
            if (attrSet == null) {
                setPaperGeomEditable(false, false);
                resolutionLabel.setVisible(false);
                formatLabel.setVisible(false);
                pageLabel.setVisible(false);
                colorLabel.setVisible(false);
            }
            bUpdating = false;
        }

        private void checkSaveData(boolean bSaveOnly) {
            bChanged = false;
            bNewValue = false;
            if (attrSet == null || !bChangeable)
                return;
            String s = (String)formatCb.getSelectedItem();
            if (!s.equals(attrSet.new_formatStr))
                 bNewValue = true;
            if (!s.equals(attrSet.formatStr))
                 bChanged = true;
            attrSet.new_formatStr = s;
            if (bDebug) {
               System.out.println("    new_formatStr: "+attrSet.new_formatStr);
            }
            VjMediaSizeObj obj = (VjMediaSizeObj)pageCb.getSelectedItem();
            if (obj != null) {
                pageLabel.setText(obj.toString());
                s = obj.getMediaName();
                if (!s.equals(attrSet.new_paperSize))
                    bNewValue = true;
                if (!s.equals(attrSet.paperSize))
                    bChanged = true;
                attrSet.new_paperSize = s;
                if (bDebug)
                    System.out.println("   new_paperSize: "+attrSet.new_paperSize);
            }
            ColorItem cobj = (ColorItem)colorCb.getSelectedItem();
            if (cobj != null) {
                colorLabel.setText(cobj.toString());
                s = (String)cobj.getName();
                if (!s.equals(attrSet.new_colorStr))
                    bNewValue = true;
                if (!s.equals(attrSet.colorStr))
                    bChanged = true;
                attrSet.new_colorStr = s;
                if (bDebug)
                    System.out.println("   new_color: "+attrSet.new_colorStr);
            }
            if (resolutionTxt.isVisible()) {
               s = resolutionTxt.getText().trim(); 
               int n1 = VjPrintUtil.getInteger(s);
               if (n1 > 10) {
                   if (bDebug)
                      System.out.println(" resolution: "+s);
                   attrSet.new_resolutionStr = s;
                   if (!s.equals(attrSet.resolutionStr))
                       bChanged = true;
               }
            }
            if (lineWidthTxt.isVisible()) {
               s = lineWidthTxt.getText().trim(); 
               int n1 = VjPrintUtil.getInteger(s);
               if (n1 >= 0) {
                   if (bDebug)
                      System.out.println(" line width: "+s);
                   if (n1 > 99)
                      attrSet.new_linewidthStr = "1";
                   else
                      attrSet.new_linewidthStr = s;
                   if (!s.equals(attrSet.linewidthStr))
                       bChanged = true;
               }
            }
            if (paperWidthTxt.isVisible()) {
               bUpdating = true;
               s = paperWidthTxt.getText().trim();
               double d;
               if (VjPrintUtil.isDoubleString(s)) {
                   d = VjPrintUtil.getDouble(s);
                   if (d > 20.0) {
                       if (!s.equals(attrSet.new_paperWidthStr)) {
                           bNewValue = true;
                           attrSet.new_paperWidthStr = s;
                           attrSet.paperWidth = d;
                       }
                   }
                   if (!s.equals(attrSet.paperWidthStr))
                       bChanged = true;
               }
               else
                   paperWidthTxt.setText(getAttr(VjPlotDef.PAPER_WIDTH));
               s = paperHeightTxt.getText().trim();
               if (VjPrintUtil.isDoubleString(s)) {
                   d = VjPrintUtil.getDouble(s);
                   if (d > 20.0) {
                       if (!s.equals(attrSet.new_paperHeightStr)) {
                           bNewValue = true;
                           attrSet.new_paperHeightStr = s;
                           attrSet.paperHeight = d;
                       }
                   }
                   if (!s.equals(attrSet.paperHeightStr))
                       bChanged = true;
               }
               else
                   paperHeightTxt.setText(getAttr(VjPlotDef.PAPER_HEIGHT));
               bUpdating = false;
            }
        }

        public void saveInfo() {
            if (attrSet == null || !bChangeable)
                return;
            if (bDebug)
               System.out.println(" media panel saveInfo... ");
            checkSaveData(true);
            if (bChanged)
                attrSet.bChanged = true;
        }

        public void itemStateChanged(ItemEvent e) {
            if (bUpdating || attrSet == null)
               return;
            if (e.getStateChange() != ItemEvent.SELECTED)
               return;
            checkSaveData(false);
            if (!bNewValue)
               return;
            Object source = e.getSource();
            if (source == pageCb) {
               changePaper();
            }
            else if (source == formatCb) {
               changeFormat();
            }
            else if (source == colorCb) {
               changeColor();
            }
            updateLayout();
        }

        private void checkChanges() {
            if (bUpdating || attrSet == null)
               return;
            checkSaveData(false);
            if (bNewValue) {
               attrSet.changeWcMax();
               updateLayout();
            }
        }

        public void actionPerformed(ActionEvent e) {
            checkChanges();
        }

        public void focusLost(FocusEvent e) {
            checkChanges();
        }

        public void focusGained(FocusEvent e) {}
    }

    private class ImgRadioButton extends JPanel {
        private JRadioButton rb;
        private JLabel lbl;

        public ImgRadioButton(String key, String img, ButtonGroup bg)
        {
            super(new FlowLayout(FlowLayout.LEADING));

            Icon icon = Util.getImageIcon(img);
            if (icon != null) {
                lbl = new JLabel(icon);
                add(lbl);
            }
            rb = new JRadioButton(key);
            bg.add(rb);
            add(rb);
        }

        public void addActionListener(ActionListener al) {
            rb.addActionListener(al);
        }

        public void setEnabled(boolean enabled) {
            rb.setEnabled(enabled);
            if (lbl != null)
                lbl.setEnabled(enabled);
        }

        public boolean isSelected() {
            return rb.isSelected();
        }

        public void setSelected(boolean selected) {
            rb.setSelected(selected);
        }
    }

    private class OrientationPanel extends JPanel implements ActionListener {
        private String strTitle;
        private boolean bChanged;
        private boolean bUpdating;

        public OrientationPanel() {
            super();

            strTitle = Util.getLabel("_Orientation", "Orientation");
            GridBagLayout gridbag = new GridBagLayout();
            GridBagConstraints c = new GridBagConstraints();

            setLayout(gridbag);
            setBorder(VjPrintUtil.createTitledBorder(strTitle));

            Insets compInsets = new Insets(3, 6, 3, 6);
            c.fill = GridBagConstraints.BOTH;
            c.insets = compInsets;
            c.weighty = 1.0;
            c.gridwidth = GridBagConstraints.REMAINDER;

            ButtonGroup bg = new ButtonGroup();
            String lblStr = Util.getLabel("_Portrait", "Portrait");
            rbPortrait = new ImgRadioButton(lblStr, "printPortrait.png", bg);
            rbPortrait.addActionListener(this);
            addToGB(rbPortrait, this, gridbag, c);
            lblStr = Util.getLabel("_Landscape", "Landscape");
            rbLandscape = new ImgRadioButton(lblStr, "printLandscape.png", bg);

            rbLandscape.addActionListener(this);
            addToGB(rbLandscape, this, gridbag, c);
            rbPortrait.setSelected(true);
        }

        public void updateInfo() {
            int raster = 0;
            if (attrSet != null)
               raster = VjPrintUtil.getInteger(getAttr(VjPlotDef.RASTER));
           
            if (raster == 0) {
                rbPortrait.setEnabled(false);
                rbLandscape.setEnabled(false);
                rbLandscape.setSelected(true);
                return;
            }
            if (bDebug)
                System.out.println("   raster: "+raster);
            bUpdating = true;
            if ((raster == 2) || (raster == 4))
                rbLandscape.setSelected(true);
            else
                rbPortrait.setSelected(true);
            rbPortrait.setEnabled(bChangeable);
            rbLandscape.setEnabled(bChangeable);
            bUpdating = false;
        }

        private void checkSaveData() {
            bChanged = false;
            String s = getAttr(VjPlotDef.RASTER);
            if (!s.equals(attrSet.rasterStr))
                bChanged = true;
            if (bDebug)
                System.out.println("   raster: "+ s);
        }
            
        private void changeOrientation() {
            bChanged = false;
            String s = getAttr(VjPlotDef.RASTER);
            int oldRaster = VjPrintUtil.getInteger(s);
            if (oldRaster == 0)
                return;
            if (bDebug)
                System.out.println("  changeOrientation ...");
            int newRaster = oldRaster;
            if (rbLandscape.isSelected()) {
                if (oldRaster < 3)
                    newRaster = 2;
                else
                    newRaster = 4;
            }
            else {
                if (attrSet.raster < 3)
                    newRaster = 1;
                else
                    newRaster = 3;
            }
            if (newRaster != oldRaster) {
                bChanged = true;
                // attrSet.setRaster(newRaster);
                changeRaster(newRaster);
            }
        }

        public void saveInfo() {
            if (attrSet == null)
                return;
            if (bDebug)
               System.out.println(" orientation panel saveInfo... ");
            checkSaveData();
            if (bChanged)
                attrSet.bChanged = true;
        }

        public void actionPerformed(ActionEvent e) {
            if (bUpdating || attrSet == null)
               return;
            changeOrientation();
            if (bChanged)
               updateLayout();
        }
    }

    private class MarginsPanel extends JPanel implements ActionListener, FocusListener {
        private boolean bChanged;
        private boolean bUpdating;
        private boolean bNewVal;

        public MarginsPanel() {
            GridBagLayout gridbag = new GridBagLayout();
            GridBagConstraints c = new GridBagConstraints();
            Insets compInsets = new Insets(3, 6, 3, 6);
            c.fill = GridBagConstraints.BOTH;
            c.weightx = 1.0;
            c.weighty = 1.0;
            c.insets = compInsets;

            setLayout(gridbag);
            String strTitle = Util.getLabel("_Paper_Margins","Paper Margins");
            setBorder(VjPrintUtil.createTitledBorder(strTitle));

            leftMarginTxt = new JTextField("", 6);
            rightMarginTxt = new JTextField("", 6);
            topMarginTxt = new JTextField("", 6);
            bottomMarginTxt = new JTextField("", 6);

            leftMarginTxt.addActionListener(this);
            rightMarginTxt.addActionListener(this);
            topMarginTxt.addActionListener(this);
            bottomMarginTxt.addActionListener(this);
            leftMarginTxt.addFocusListener(this);
            rightMarginTxt.addFocusListener(this);
            topMarginTxt.addFocusListener(this);
            bottomMarginTxt.addFocusListener(this);

            c.gridwidth = GridBagConstraints.RELATIVE;
            String lblStr = Util.getLabel("_left", "Left")+unitsStr;
            JLabel label = new JLabel(lblStr, JLabel.LEADING);
            label.setLabelFor(leftMarginTxt);
            addToGB(label, this, gridbag, c);

            c.gridwidth = GridBagConstraints.REMAINDER;
            lblStr = Util.getLabel("_right", "Right")+unitsStr;
            label = new JLabel(lblStr, JLabel.LEADING);
            label.setLabelFor(rightMarginTxt);
            addToGB(label, this, gridbag, c);

            c.gridwidth = GridBagConstraints.RELATIVE;
            addToGB(leftMarginTxt, this, gridbag, c);

            c.gridwidth = GridBagConstraints.REMAINDER;
            addToGB(rightMarginTxt, this, gridbag, c);

            c.gridwidth = GridBagConstraints.RELATIVE;
            lblStr = Util.getLabel("_top", "Top")+unitsStr;
            label = new JLabel(lblStr, JLabel.LEADING);
            label.setLabelFor(topMarginTxt);
            addToGB(label, this, gridbag, c);

            c.gridwidth = GridBagConstraints.REMAINDER;
            lblStr = Util.getLabel("_bottom", "Bottom")+unitsStr;
            label = new JLabel(lblStr, JLabel.LEADING);
            label.setLabelFor(bottomMarginTxt);
            addToGB(label, this, gridbag, c);

            c.gridwidth = GridBagConstraints.RELATIVE;
            addToGB(topMarginTxt, this, gridbag, c);

            c.gridwidth = GridBagConstraints.REMAINDER;
            addToGB(bottomMarginTxt, this, gridbag, c);
        }

        public void updateInfo() {
            bUpdating = true;
            leftMarginTxt.setEditable(bChangeable);
            rightMarginTxt.setEditable(bChangeable);
            topMarginTxt.setEditable(bChangeable);
            bottomMarginTxt.setEditable(bChangeable);

            leftMarginTxt.setText(getAttr(VjPlotDef.LEFT_EDGE));
            rightMarginTxt.setText(getAttr(VjPlotDef.RIGHT_EDGE));
            topMarginTxt.setText(getAttr(VjPlotDef.TOP_EDGE));
            bottomMarginTxt.setText(getAttr(VjPlotDef.BOTTOM_EDGE));
            bUpdating = false;
        }

        private void checkSaveData() {
            bChanged = false;
            bNewVal = false;
            if (attrSet == null)
                return;
            String vs;
            String s = leftMarginTxt.getText().trim();
            double d;
            bUpdating = true;
            if (VjPrintUtil.isDoubleString(s)) {
                d = VjPrintUtil.getDouble(s);
                if (d >= 0) {
                    vs = getAttr(VjPlotDef.LEFT_EDGE);
                    if (!s.equals(attrSet.left_edgeStr))
                        bChanged = true;
                    if (!s.equals(vs))
                        bNewVal = true;
                    attrSet.new_left_edgeStr = s;
                    attrSet.leftMargin = d;
                }
            }
            else
                leftMarginTxt.setText(getAttr(VjPlotDef.LEFT_EDGE));
            s = rightMarginTxt.getText().trim();
            if (VjPrintUtil.isDoubleString(s)) {
                d = VjPrintUtil.getDouble(s);
                if (d >= 0) {
                    vs = getAttr(VjPlotDef.RIGHT_EDGE);
                    if (!s.equals(attrSet.right_edgeStr))
                        bChanged = true;
                    if (!s.equals(vs))
                        bNewVal = true;
                    attrSet.new_right_edgeStr = s;
                    attrSet.rightMargin = d;
                }
            }
            else
                rightMarginTxt.setText(getAttr(VjPlotDef.RIGHT_EDGE));
            s = topMarginTxt.getText().trim();
            if (VjPrintUtil.isDoubleString(s)) {
                d = VjPrintUtil.getDouble(s);
                if (d >= 0) {
                    vs = getAttr(VjPlotDef.TOP_EDGE);
                    if (!s.equals(attrSet.top_edgeStr))
                        bChanged = true;
                    if (!s.equals(vs))
                        bNewVal = true;
                    attrSet.new_top_edgeStr = s;
                    attrSet.topMargin = d;
                }
            }
            else
                topMarginTxt.setText(getAttr(VjPlotDef.TOP_EDGE));
            s = bottomMarginTxt.getText().trim();
            if (VjPrintUtil.isDoubleString(s)) {
                d = VjPrintUtil.getDouble(s);
                if (d >= 0) {
                    vs = getAttr(VjPlotDef.BOTTOM_EDGE);
                    if (!s.equals(vs))
                        bNewVal = true;
                    if (!s.equals(attrSet.bottom_edgeStr))
                        bChanged = true;
                    attrSet.new_bottom_edgeStr = s;
                    attrSet.bottomMargin = d;
                }
            }
            else
                bottomMarginTxt.setText(getAttr(VjPlotDef.BOTTOM_EDGE));
            bUpdating = false;
        }

        public void saveInfo() {
            if (bDebug)
               System.out.println(" margin panel saveInfo... ");
            checkSaveData();
            if (bChanged)
                attrSet.bChanged = true;
        }

        private void checkChanges() {
            if (bUpdating)
                return;
            checkSaveData();
            if (bNewVal) {
               attrSet.changeWcMax();
               updateLayout();
            }
        }

        public void actionPerformed(ActionEvent e) {
            checkChanges();
        }

        public void focusLost(FocusEvent e) {
            checkChanges();
        }

        public void focusGained(FocusEvent e) {}

    }

    private class WcmaxPanel extends JPanel implements ActionListener, FocusListener {
        private boolean bChanged;
        private boolean bUpdating;
        private boolean bNewVal;

        public WcmaxPanel() {
            GridBagLayout gridbag = new GridBagLayout();
            GridBagConstraints c = new GridBagConstraints();
            Insets compInsets = new Insets(3, 6, 3, 24);
            c.fill = GridBagConstraints.BOTH;
            c.weightx = 1.0;
            c.weighty = 1.0;
            c.insets = compInsets;

            setLayout(gridbag);
            String strTitle = Util.getLabel("_Spectrum_Area","Spectrum Area");
            setBorder(VjPrintUtil.createTitledBorder(strTitle));

            wcmaxTxt = new JTextField("", 6);
            wc2maxTxt = new JTextField("", 6);

            wcmaxTxt.addActionListener(this);
            wc2maxTxt.addActionListener(this);
            wcmaxTxt.addFocusListener(this);
            wc2maxTxt.addFocusListener(this);

            c.gridwidth = GridBagConstraints.REMAINDER;
            JLabel label = new JLabel("Wcmaxmax" + unitsStr, JLabel.LEADING);
            label.setLabelFor(wcmaxTxt);
            addToGB(label, this, gridbag, c);

            c.gridwidth = GridBagConstraints.REMAINDER;
            addToGB(wcmaxTxt, this, gridbag, c);

            c.gridwidth = GridBagConstraints.REMAINDER;
            label = new JLabel("Wc2maxmax" + unitsStr, JLabel.LEADING);
            label.setLabelFor(wc2maxTxt);
            addToGB(label, this, gridbag, c);

            c.gridwidth = GridBagConstraints.REMAINDER;
            addToGB(wc2maxTxt, this, gridbag, c);
        }

        public void updateInfo() {
            bUpdating = true;
            wcmaxTxt.setEditable(bChangeable);
            wc2maxTxt.setEditable(bChangeable);
            /**
            if (bNoneDevice) {
                wcmaxTxt.setText("0");
                wc2maxTxt.setText("0");
            }
            else {
            ***/
                wcmaxTxt.setText(getAttr(VjPlotDef.WC_MAX));
                wc2maxTxt.setText(getAttr(VjPlotDef.WC2_MAX));
            // }
            bUpdating = false;
        }

        private void checkSaveData() {
            bChanged = false;
            bNewVal = false;
            if (attrSet == null)
                return;
            String vs;
            String s = wcmaxTxt.getText().trim();
            double d = 0;
            bUpdating = true;
            if (VjPrintUtil.isDoubleString(s)) {
                d = VjPrintUtil.getDouble(s);
                if (d > 10) {
                    vs = getAttr(VjPlotDef.WC_MAX);
                    if (!s.equals(attrSet.wcmaxmaxStr))
                        bChanged = true;
                    if (!s.equals(vs))
                        bNewVal = true;
                    attrSet.new_wcmaxmaxStr = s;
                    attrSet.wcmaxmax = d;
                }
            }
            if (d < 10)
                wcmaxTxt.setText(getAttr(VjPlotDef.WC_MAX));
            d = 0;
            s = wc2maxTxt.getText().trim();
            if (VjPrintUtil.isDoubleString(s)) {
                d = VjPrintUtil.getDouble(s);
                if (d > 10) {
                    vs = getAttr(VjPlotDef.WC2_MAX);
                    if (!s.equals(attrSet.wc2maxmaxStr))
                        bChanged = true;
                    if (!s.equals(vs))
                        bNewVal = true;
                    attrSet.new_wc2maxmaxStr = s;
                    attrSet.wc2maxmax = d;
                }
            }
            if (d < 10)
                wc2maxTxt.setText(getAttr(VjPlotDef.WC2_MAX));
            bUpdating = false;
        }

        public void saveInfo() {
            if (bDebug)
               System.out.println(" wc wc2 panel saveInfo... ");
            checkSaveData();
            if (bChanged)
                attrSet.bChanged = true;
        }

        private void checkChanges() {
            if (bUpdating)
               return;
            checkSaveData();
            if (bNewVal) {
               attrSet.changeWcMax();
               updateLayout();
            }
        }

        public void actionPerformed(ActionEvent e) {
            checkChanges();
        }

        public void focusLost(FocusEvent e) {
            checkChanges();
        }

        public void focusGained(FocusEvent e) {}

    }

    private class ButtonPanel extends JPanel implements ActionListener {

        public ButtonPanel() {
            setLayout(new SimpleH2Layout(SimpleH2Layout.CENTER, 20, 5, false));
            String lblStr = Util.getLabel("_Reset_to_default", "Reset to default");
            resetButton = new JButton(lblStr);
            add(resetButton);
            resetButton.addActionListener(this);
        }

        public void actionPerformed(ActionEvent e) {
            if (attrSet == null)
                return;
            attrSet.resetToOrigin();
            updateInfo();
        }
    }
        
    private class PagePanel extends JPanel {
        private VjPlotterPage page;
        private int GAP;
        private int pw, ph;
        private Font myFont;
        private Color ftColor;
        private FontMetrics fm;
        private String pwStr = "";
        private String phStr = "";


        public PagePanel() {

            page = new VjPlotterPage();
            add(page);
            new VjPageLayout();
            setLayout(null);
            ftColor = Color.blue;
            // myFont = new Font(Font.SERIF, Font.BOLD, 10);
            myFont = new Font(Font.DIALOG, Font.BOLD, 10);
            fm = getFontMetrics(myFont);
            this.GAP = fm.getHeight() + 8;
        }

        private void  setPageAttribute() {
            page.setPageAttribute(attrSet, bChangeable);
        }

        private void updateInfo() {
            if (attrSet != null) {
                boolean bLandscape = false;
                boolean bSwap = false;
                if (attrSet.raster == 2 || attrSet.raster == 4)
                    bLandscape = true;
                else if (attrSet.raster == 0) {
                    if (attrSet.wcmaxmax > attrSet.wc2maxmax)
                       bLandscape = true;
                }
                if (bLandscape) {
                    if (attrSet.actualHeight > attrSet.actualWidth)
                       bSwap = true;
                }
                else {
                    if (attrSet.actualWidth > attrSet.actualHeight)
                       bSwap = true;
                }
                if (bSwap) {
                    attrSet.dispWidth = attrSet.actualHeight;
                    attrSet.dispHeight = attrSet.actualWidth;
                }
                else {
                    attrSet.dispWidth = attrSet.actualWidth;
                    attrSet.dispHeight = attrSet.actualHeight;
                }
                double d = VjPrintUtil.getDouble(attrSet.dispWidth / 25.4);
                pwStr = Double.toString(d)+"\"";
                d = VjPrintUtil.getDouble(attrSet.dispHeight / 25.4);
                phStr = Double.toString(d)+"\"";
            }
            page.updateInfo();
            setPageSize();
            repaint();
        }

        private void setPageSize() {
            Dimension size = getSize();
            if (size.width < 10 || size.height < 10)
                 return;
            if (attrSet == null) {
                page.setBounds(0, 0, 0, 0);
                return;
            }

            double r;
            double dw, dh;
            if (attrSet.dispWidth > 0.0)
                r = attrSet.dispHeight / attrSet.dispWidth;
            else
                r = 1.0;
            if (r <= 0.0)
                r = 1.0;
            dw = size.width - GAP * 2;
            dh = size.height - GAP * 2;
            double r1 = 1.0;
            double rh = dh;
            while (true) {
               rh = dw * r * r1;
               if (rh <= dh)
                   break;
               r1 = r1 - (rh - dh) / rh;
               if (r1 < 0.2)
                  break;
            }
            dw = dw * r1;
            dh = dw * r;
            pw = (int) dw;
            ph = (int) dh;
            page.setBounds(GAP, GAP, pw, ph);
        }

        public void setBounds(int x, int y, int w, int h) {
            super.setBounds(x, y, w, h);  
            setPageSize(); 
        }

        public void paint(Graphics g) {
            super.paint(g);
            if (attrSet == null)
               return;
            Graphics2D g2d = (Graphics2D) g;
            g2d.setColor(ftColor);
            g2d.setFont(myFont);
            int w = fm.stringWidth(pwStr);
            int x = GAP + (pw - w) / 2;
            int y = GAP + ph + fm.getHeight() + 2;
            g2d.drawString(pwStr, x, y);
            w = fm.stringWidth(phStr);
            x = fm.getHeight() + 2;
            y = GAP + w + (ph - w) / 2;
            g2d.translate(x, y);
            g2d.rotate(-2 * Math.PI / 4);
            g2d.drawString(phStr, 0, 0);
        }
    }

    private class ColorItem  {

        private String label;
        private String name;

        public ColorItem(String l, String name) {
            this.label = l;
            this.name = name;
        }

        public String getName() {
            return name;
        }

        public String toString() {
            if (label != null)
                return label;
            return name;
        }
    }
} // end of VjPlotterLayout

